#include "value_methods.h"

#include "big_int.h"
#include "borrowed_cells.h"
#include "case_mapping.h"
#include "cycle_collector.h"
#include "dialect.h"
#include "engine_errors.h"
#include "expression_evaluator.h"
#include "host_object.h"
#include "internal_value.h"
#include "markup.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "stable_sort_values.h"
#include "template_dict.h"
#include "undefined.h"
#include "unicode_case.h"
#include "unicode_printable.h"
#include "unicode_tables.h"
#include "value_visitors.h"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{
inline namespace v2
{
namespace methods
{
namespace
{
[[noreturn]] void Raise(const std::string& message)
{
    throw TemplateRuntimeError(message);
}

std::string TypeName(const InternalValue& val)
{
    return Apply<visitors::PythonTypeNameGetter>(val);
}

// Python's == (1 == 1.0, 'a' == 'a' across string kinds)
bool Equals(const InternalValue& lhs, const InternalValue& rhs)
{
    return ConvertToBool(Apply2<visitors::BinaryMathOperation>(lhs, rhs, BinaryExpression::LogicalEq));
}

// Python hashes a dict key before it looks at the dict: a list or a dict is a TypeError
void CheckHashable(const InternalValue& key)
{
    if (const char* type = UnhashableType(key))
    {
        ThrowUnhashable(type);
    }
}

// Positional argument `idx`, or the keyword argument `kwName` when the method takes one
const InternalValue* Arg(const CallParams& params, size_t idx, const char* kwName = nullptr)
{
    if (idx < params.posParams.size())
    {
        return &params.posParams[idx];
    }
    if (kwName)
    {
        auto p = params.kwParams.find(kwName);
        if (p != params.kwParams.end())
        {
            return &p->second;
        }
    }
    return nullptr;
}

const InternalValue* ArgOrNone(const CallParams& params, size_t idx, const char* kwName = nullptr)
{
    const auto* arg = Arg(params, idx, kwName);
    return !arg || arg->IsNone() ? nullptr : arg;
}

// Python's arity check; keyword arguments are accepted only where Python takes them
void CheckArgs(const CallParams& params, const char* name, size_t minArgs, size_t maxArgs, std::initializer_list<const char*> kwNames = {})
{
    for (const auto& kw : params.kwParams)
    {
        if (std::find_if(kwNames.begin(), kwNames.end(), [&kw](const char* n) { return kw.first == n; }) == kwNames.end())
        {
            Raise(std::string(name) + "() got an unexpected keyword argument '" + kw.first + "'");
        }
    }
    auto count = params.posParams.size() + params.kwParams.size();
    if (count < minArgs || params.posParams.size() > maxArgs)
    {
        if (minArgs == maxArgs)
        {
            Raise(fmt::format("{}() takes exactly {} argument{} ({} given)", name, minArgs, minArgs == 1 ? "" : "s", count));
        }
        if (count < minArgs)
        {
            Raise(fmt::format("{}() takes at least {} argument{} ({} given)", name, minArgs, minArgs == 1 ? "" : "s", count));
        }
        Raise(fmt::format("{}() takes at most {} argument{} ({} given)", name, maxArgs, maxArgs == 1 ? "" : "s", count));
    }
}

// A width or precision a method pads to counts against the same cap as every other string
// the template builds (maxSequenceSize, docs/tasks/0263); Python raises MemoryError or
// OverflowError well before a template could allocate this much
int64_t WidthArg(int64_t width)
{
    if (width > 0)
    {
        CheckSequenceSize(static_cast<uint64_t>(width), "a padded string");
    }
    return width;
}

int64_t IntArg(const InternalValue& val, const char* name)
{
    if (const auto* i = GetIf<int64_t>(&val))
    {
        return *i;
    }
    if (const auto* b = GetIf<bool>(&val))
    {
        return *b ? 1 : 0;
    }
    if (val.Kind() == ValueKind::BigInt)
    {
        ThrowIndexOverflow();
    }
    Raise(std::string(name) + "(): '" + TypeName(val) + "' object cannot be interpreted as an integer");
}

// Python's slice bounds: a negative index counts from the end, then it is clamped
size_t SliceIndex(const InternalValue* val, size_t len, size_t def, const char* name)
{
    if (!val || val->IsNone())
    {
        return def;
    }
    auto idx = IntArg(*val, name);
    if (idx < 0)
    {
        idx += static_cast<int64_t>(len);
    }
    return static_cast<size_t>(std::min<int64_t>(std::max<int64_t>(idx, 0), static_cast<int64_t>(len)));
}

InternalValue MakeTuple(InternalValueList items)
{
    return ListAdapter::CreateAdapter(std::move(items)).MarkAsTuple();
}

// ---------------------------------------------------------------------------------------
// str

// The ASCII fast paths of the case methods; other strings take case_mapping.h's full mappings
inline bool IsAsciiUpper(uint32_t cp)
{
    return cp >= 'A' && cp <= 'Z';
}
inline bool IsAsciiLower(uint32_t cp)
{
    return cp >= 'a' && cp <= 'z';
}
inline bool IsAlphaCp(uint32_t cp)
{
    return unicode::IsWordChar(cp) && !unicode::IsDecimal(cp) && cp != '_';
}

template<typename CharT>
struct StrOps
{
    using Str = std::basic_string<CharT>;
    using View = std::basic_string_view<CharT>;
    using Chars = std::vector<View>;

    static InternalValue Result(Str str) { return InternalValue(TargetString(std::move(str))); }

    static Str StrArg(View self, const InternalValue& val, const char* name)
    {
        auto str = GetAsSameString(self, val);
        if (!str || !IsStringValue(val))
        {
            Raise(std::string(name) + "() argument must be str, not " + TypeName(val));
        }
        return std::move(*str);
    }

    // The characters [b, e) of chars as a string
    static Str Join(const Chars& chars, size_t b, size_t e)
    {
        if (b >= e)
        {
            return Str();
        }
        // The characters are views into one string, so [b, e) is contiguous
        const auto* end = chars[e - 1].data() + chars[e - 1].size();
        return Str(chars[b].data(), static_cast<size_t>(end - chars[b].data())); // NOLINT(bugprone-suspicious-stringview-data-usage): sized
    }

    // The code unit offset of character idx
    static size_t UnitOffset(View self, const Chars& chars, size_t idx)
    {
        return idx >= chars.size() ? self.size() : static_cast<size_t>(chars[idx].data() - self.data());
    }

    // The character index of a code unit offset
    static size_t CharIndex(View self, const Chars& chars, size_t offset)
    {
        auto p = std::lower_bound(chars.begin(), chars.end(), self.data() + offset, [](View ch, const CharT* pos) { return ch.data() < pos; });
        return static_cast<size_t>(p - chars.begin());
    }

    static bool IsAscii(View self)
    {
        return std::all_of(self.begin(), self.end(), [](CharT ch) { return CodeUnit(ch) < 0x80; });
    }

    // fn of each character of an ASCII string
    template<typename Fn>
    static Str MapChars(View self, const Fn& fn)
    {
        Str result;
        result.reserve(self.size());
        for (auto ch : self)
        {
            result.push_back(static_cast<CharT>(fn(CodeUnit(ch))));
        }
        return result;
    }

    static InternalValue Upper(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "upper", 0, 0);
        if (!IsAscii(self))
        {
            return Result(UpperUnicode(self));
        }
        return Result(MapChars(self, [](uint32_t cp) { return IsAsciiLower(cp) ? cp - 32 : cp; }));
    }
    static InternalValue Lower(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "lower", 0, 0);
        if (!IsAscii(self))
        {
            return Result(LowerUnicode(self));
        }
        return Result(MapChars(self, [](uint32_t cp) { return IsAsciiUpper(cp) ? cp + 32 : cp; }));
    }
    // str.casefold(): lower() without the final sigma rule, and full folding ('ß' is 'ss')
    static InternalValue Casefold(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "casefold", 0, 0);
        if (!IsAscii(self))
        {
            Str result;
            CaseChars<CharT> chars(self);
            for (size_t idx = 0; idx != chars.cps.size(); ++idx)
            {
                AppendMapped(result, chars, idx, unicode::ToFold);
            }
            return Result(std::move(result));
        }
        return Result(MapChars(self, [](uint32_t cp) { return IsAsciiUpper(cp) ? cp + 32 : cp; }));
    }
    static InternalValue Swapcase(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "swapcase", 0, 0);
        if (!IsAscii(self))
        {
            return Result(SwapcaseUnicode(self));
        }
        return Result(MapChars(self, [](uint32_t cp) {
            if (IsAsciiUpper(cp))
            {
                return cp + 32;
            }
            return IsAsciiLower(cp) ? cp - 32 : cp;
        }));
    }
    static InternalValue Title(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "title", 0, 0);
        if (!IsAscii(self))
        {
            return Result(PythonTitleUnicode(self));
        }
        bool prevCased = false;
        return Result(MapChars(self, [&prevCased](uint32_t cp) {
            bool isCased = IsAsciiUpper(cp) || IsAsciiLower(cp);
            if (isCased)
            {
                cp = prevCased ? (IsAsciiUpper(cp) ? cp + 32 : cp) : (IsAsciiLower(cp) ? cp - 32 : cp);
            }
            prevCased = isCased;
            return cp;
        }));
    }
    static InternalValue Capitalize(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "capitalize", 0, 0);
        if (!IsAscii(self))
        {
            return Result(CapitalizeUnicode(self));
        }
        bool first = true;
        return Result(MapChars(self, [&first](uint32_t cp) {
            auto mapped = first ? (IsAsciiLower(cp) ? cp - 32 : cp) : (IsAsciiUpper(cp) ? cp + 32 : cp);
            first = false;
            return mapped;
        }));
    }

    static InternalValue StripImpl(View self, const CallParams& params, const char* name, bool left, bool right)
    {
        CheckArgs(params, name, 0, 1);
        auto chars = SplitCodePoints(self);
        std::function<bool(View)> isStripped = [](View ch) { return unicode::IsSpace(CodePointValue(ch)); };
        Str stripChars;
        Chars stripSet;
        if (const auto* arg = ArgOrNone(params, 0))
        {
            stripChars = StrArg(self, *arg, name);
            stripSet = SplitCodePoints(View(stripChars));
            isStripped = [&stripSet](View ch) { return std::find(stripSet.begin(), stripSet.end(), ch) != stripSet.end(); };
        }
        size_t b = 0;
        size_t e = chars.size();
        while (left && b < e && isStripped(chars[b]))
        {
            ++b;
        }
        while (right && e > b && isStripped(chars[e - 1]))
        {
            --e;
        }
        return Result(Join(chars, b, e));
    }
    static InternalValue Strip(View self, const CallParams& params, RenderContext&) { return StripImpl(self, params, "strip", true, true); }
    static InternalValue Lstrip(View self, const CallParams& params, RenderContext&) { return StripImpl(self, params, "lstrip", true, false); }
    static InternalValue Rstrip(View self, const CallParams& params, RenderContext&) { return StripImpl(self, params, "rstrip", false, true); }

    static InternalValue MakeList(std::vector<Str> parts)
    {
        InternalValueList items;
        items.reserve(parts.size());
        for (auto& p : parts)
        {
            items.emplace_back(TargetString(std::move(p)));
        }
        return ListAdapter::CreateAdapter(std::move(items));
    }

    // str.split() without a separator, from the left
    static std::vector<Str> SplitWhitespace(const Chars& chars, int64_t maxSplit)
    {
        auto isSpace = [&chars](size_t idx) { return unicode::IsSpace(CodePointValue(chars[idx])); };
        std::vector<Str> parts;
        size_t pos = 0;
        while (true)
        {
            while (pos < chars.size() && isSpace(pos))
            {
                ++pos;
            }
            if (pos == chars.size())
            {
                break;
            }
            if (maxSplit >= 0 && static_cast<int64_t>(parts.size()) == maxSplit)
            {
                size_t end = chars.size();
                parts.push_back(Join(chars, pos, end));
                break;
            }
            size_t start = pos;
            while (pos < chars.size() && !isSpace(pos))
            {
                ++pos;
            }
            parts.push_back(Join(chars, start, pos));
        }
        return parts;
    }

    // str.rsplit() without a separator
    static std::vector<Str> RsplitWhitespace(const Chars& chars, int64_t maxSplit)
    {
        auto isSpace = [&chars](size_t idx) { return unicode::IsSpace(CodePointValue(chars[idx])); };
        std::vector<Str> parts;
        size_t pos = chars.size();
        while (true)
        {
            while (pos > 0 && isSpace(pos - 1))
            {
                --pos;
            }
            if (pos == 0)
            {
                break;
            }
            if (maxSplit >= 0 && static_cast<int64_t>(parts.size()) == maxSplit)
            {
                parts.push_back(Join(chars, 0, pos));
                break;
            }
            size_t end = pos;
            while (pos > 0 && !isSpace(pos - 1))
            {
                --pos;
            }
            parts.push_back(Join(chars, pos, end));
        }
        std::reverse(parts.begin(), parts.end());
        return parts;
    }

    // str.split(sep), from the left
    static std::vector<Str> SplitOnSep(View self, const Str& sep, int64_t maxSplit)
    {
        std::vector<Str> parts;
        size_t start = 0;
        while (maxSplit < 0 || static_cast<int64_t>(parts.size()) < maxSplit)
        {
            auto p = self.find(sep, start);
            if (p == View::npos)
            {
                break;
            }
            parts.emplace_back(self.substr(start, p - start));
            start = p + sep.size();
        }
        parts.emplace_back(self.substr(start));
        return parts;
    }

    // str.rsplit(sep)
    static std::vector<Str> RsplitOnSep(View self, const Str& sep, int64_t maxSplit)
    {
        std::vector<Str> parts;
        size_t end = self.size();
        while (maxSplit < 0 || static_cast<int64_t>(parts.size()) < maxSplit)
        {
            if (end < sep.size())
            {
                break;
            }
            auto p = self.substr(0, end).rfind(sep);
            if (p == View::npos)
            {
                break;
            }
            parts.emplace_back(self.substr(p + sep.size(), end - p - sep.size()));
            end = p;
        }
        parts.emplace_back(self.substr(0, end));
        std::reverse(parts.begin(), parts.end());
        return parts;
    }

    static InternalValue SplitImpl(View self, const CallParams& params, const char* name, bool fromRight)
    {
        CheckArgs(params, name, 0, 2, { "sep", "maxsplit" });
        const auto* sepArg = ArgOrNone(params, 0, "sep");
        const auto* maxArg = Arg(params, 1, "maxsplit");
        int64_t maxSplit = !maxArg ? -1 : IntArg(*maxArg, name);

        if (!sepArg)
        {
            // Runs of whitespace separate; leading and trailing whitespace is dropped
            auto chars = SplitCodePoints(self);
            return MakeList(fromRight ? RsplitWhitespace(chars, maxSplit) : SplitWhitespace(chars, maxSplit));
        }

        auto sep = StrArg(self, *sepArg, name);
        if (sep.empty())
        {
            Raise("empty separator");
        }
        return MakeList(fromRight ? RsplitOnSep(self, sep, maxSplit) : SplitOnSep(self, sep, maxSplit));
    }
    static InternalValue Split(View self, const CallParams& params, RenderContext&) { return SplitImpl(self, params, "split", false); }
    static InternalValue Rsplit(View self, const CallParams& params, RenderContext&) { return SplitImpl(self, params, "rsplit", true); }

    static InternalValue Splitlines(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "splitlines", 0, 1, { "keepends" });
        const auto* keepArg = Arg(params, 0, "keepends");
        bool keepEnds = keepArg != nullptr && ConvertToBool(*keepArg);
        auto chars = SplitCodePoints(self);
        std::vector<Str> parts;
        size_t start = 0;
        for (size_t pos = 0; pos < chars.size(); ++pos)
        {
            auto cp = CodePointValue(chars[pos]);
            bool isBreak = (cp >= 0x0A && cp <= 0x0D) || (cp >= 0x1C && cp <= 0x1E) || cp == 0x85 || cp == 0x2028 || cp == 0x2029;
            if (!isBreak)
            {
                continue;
            }
            size_t end = pos + 1;
            if (cp == '\r' && end < chars.size() && CodePointValue(chars[end]) == '\n')
            {
                ++end;
            }
            parts.push_back(Join(chars, start, keepEnds ? end : pos));
            start = end;
            pos = end - 1;
        }
        if (start < chars.size())
        {
            parts.push_back(Join(chars, start, chars.size()));
        }
        return MakeList(std::move(parts));
    }

    static InternalValue JoinMethod(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "join", 1, 1);
        bool isConverted = false;
        auto list = ConvertToList(params.posParams[0], isConverted, false);
        if (!isConverted)
        {
            Raise("can only join an iterable");
        }
        Str result;
        size_t idx = 0;
        for (const auto& item : list)
        {
            auto str = GetAsSameString(self, item);
            if (!str || !IsStringValue(item))
            {
                Raise(fmt::format("sequence item {}: expected str instance, {} found", idx, TypeName(item)));
            }
            CheckSequenceSize(uint64_t{ result.size() } + (idx != 0 ? self.size() : 0) + str->size(), "a joined string");
            if (idx++ != 0)
            {
                result.append(self.begin(), self.end());
            }
            result += *str;
        }
        return Result(std::move(result));
    }

    static InternalValue Replace(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "replace", 2, 3, { "count" });
        auto oldStr = StrArg(self, params.posParams[0], "replace");
        auto newStr = StrArg(self, params.posParams[1], "replace");
        const auto* countArg = Arg(params, 2, "count");
        int64_t count = !countArg ? -1 : IntArg(*countArg, "replace");
        Str result;
        if (oldStr.empty())
        {
            // An empty pattern matches before every character and at the end
            auto chars = SplitCodePoints(self);
            for (size_t n = 0; n <= chars.size(); ++n)
            {
                if (count < 0 || static_cast<int64_t>(n) < count)
                {
                    CheckSequenceSize(uint64_t{ result.size() } + newStr.size() + (n < chars.size() ? chars[n].size() : 0),
                                      "a replaced string");
                    result += newStr;
                }
                if (n < chars.size())
                {
                    result.append(chars[n].begin(), chars[n].end());
                }
            }
            return Result(std::move(result));
        }
        size_t start = 0;
        for (int64_t done = 0; count < 0 || done < count; ++done)
        {
            auto p = self.find(oldStr, start);
            if (p == View::npos)
            {
                break;
            }
            CheckSequenceSize(uint64_t{ result.size() } + (p - start) + newStr.size(), "a replaced string");
            result.append(self.begin() + static_cast<std::ptrdiff_t>(start), self.begin() + static_cast<std::ptrdiff_t>(p));
            result += newStr;
            start = p + oldStr.size();
        }
        result.append(self.begin() + static_cast<std::ptrdiff_t>(start), self.end());
        return Result(std::move(result));
    }

    // self[start:end] for the methods that take optional start and end arguments
    static View Window(View self, const Chars& chars, const CallParams& params, size_t firstIdx, const char* name, size_t* startChar = nullptr)
    {
        auto start = SliceIndex(Arg(params, firstIdx), chars.size(), 0, name);
        auto end = SliceIndex(Arg(params, firstIdx + 1), chars.size(), chars.size(), name);
        if (startChar)
        {
            *startChar = start;
        }
        if (end < start)
        {
            end = start;
        }
        auto b = UnitOffset(self, chars, start);
        return self.substr(b, UnitOffset(self, chars, end) - b);
    }

    static InternalValue AffixImpl(View self, const CallParams& params, const char* name, bool atStart)
    {
        CheckArgs(params, name, 1, 3);
        auto chars = SplitCodePoints(self);
        // A start beyond the end matches nothing, not even an empty affix
        if (const auto* startArg = ArgOrNone(params, 1))
        {
            auto start = IntArg(*startArg, name);
            if (start > static_cast<int64_t>(chars.size()))
            {
                return InternalValue(false);
            }
        }
        auto window = Window(self, chars, params, 1, name);
        auto matches = [&](const InternalValue& affixVal) {
            auto affix = GetAsSameString(self, affixVal);
            if (!affix || !IsStringValue(affixVal))
            {
                Raise(std::string(name) + " first arg must be str or a tuple of str, not " + TypeName(affixVal));
            }
            if (affix->size() > window.size())
            {
                return false;
            }
            return atStart ? window.substr(0, affix->size()) == View(*affix) : window.substr(window.size() - affix->size()) == View(*affix);
        };
        const auto& affixes = params.posParams[0];
        if (auto list = AsList(affixes))
        {
            if (!list->IsTuple())
            {
                Raise(std::string(name) + " first arg must be str or a tuple of str, not list");
            }
            for (const auto& item : *list)
            {
                if (matches(item))
                {
                    return InternalValue(true);
                }
            }
            return InternalValue(false);
        }
        return InternalValue(matches(affixes));
    }
    static InternalValue Startswith(View self, const CallParams& params, RenderContext&) { return AffixImpl(self, params, "startswith", true); }
    static InternalValue Endswith(View self, const CallParams& params, RenderContext&) { return AffixImpl(self, params, "endswith", false); }

    static int64_t FindImpl(View self, const CallParams& params, const char* name, bool fromRight)
    {
        CheckArgs(params, name, 1, 3);
        auto sub = StrArg(self, params.posParams[0], name);
        auto chars = SplitCodePoints(self);
        if (const auto* startArg = ArgOrNone(params, 1))
        {
            auto start = IntArg(*startArg, name);
            if (start > static_cast<int64_t>(chars.size()))
            {
                return -1;
            }
        }
        size_t startChar = 0;
        auto window = Window(self, chars, params, 1, name, &startChar);
        if (sub.empty())
        {
            return static_cast<int64_t>(fromRight ? CharIndex(self, chars, static_cast<size_t>(window.data() - self.data()) + window.size()) : startChar);
        }
        auto p = fromRight ? window.rfind(sub) : window.find(sub);
        if (p == View::npos)
        {
            return -1;
        }
        return static_cast<int64_t>(CharIndex(self, chars, static_cast<size_t>(window.data() - self.data()) + p));
    }
    static InternalValue Find(View self, const CallParams& params, RenderContext&) { return FindImpl(self, params, "find", false); }
    static InternalValue Rfind(View self, const CallParams& params, RenderContext&) { return FindImpl(self, params, "rfind", true); }
    static InternalValue Index(View self, const CallParams& params, RenderContext&)
    {
        auto result = FindImpl(self, params, "index", false);
        if (result < 0)
        {
            Raise("substring not found");
        }
        return result;
    }
    static InternalValue Rindex(View self, const CallParams& params, RenderContext&)
    {
        auto result = FindImpl(self, params, "rindex", true);
        if (result < 0)
        {
            Raise("substring not found");
        }
        return result;
    }

    static InternalValue Count(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "count", 1, 3);
        auto sub = StrArg(self, params.posParams[0], "count");
        auto chars = SplitCodePoints(self);
        if (const auto* startArg = ArgOrNone(params, 1))
        {
            if (IntArg(*startArg, "count") > static_cast<int64_t>(chars.size()))
            {
                return static_cast<int64_t>(0);
            }
        }
        auto window = Window(self, chars, params, 1, "count");
        if (sub.empty())
        {
            return static_cast<int64_t>(CodePointCount(window) + 1);
        }
        int64_t result = 0;
        for (auto p = window.find(sub); p != View::npos; p = window.find(sub, p + sub.size()))
        {
            ++result;
        }
        return result;
    }

    template<typename Pred>
    static InternalValue AllChars(View self, const CallParams& params, const char* name, const Pred& pred)
    {
        CheckArgs(params, name, 0, 0);
        auto chars = SplitCodePoints(self);
        if (chars.empty())
        {
            return InternalValue(false);
        }
        return InternalValue(std::all_of(chars.begin(), chars.end(), [&pred](View ch) { return pred(CodePointValue(ch)); }));
    }
    static InternalValue Isdigit(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isdigit", unicode::IsDigit); }
    static InternalValue Isdecimal(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isdecimal", unicode::IsDecimal); }
    static InternalValue Isnumeric(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isnumeric", unicode::IsNumeric); }
    // isascii() and isprintable() hold for the empty string, unlike the other tests
    static InternalValue Isascii(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "isascii", 0, 0);
        return InternalValue(IsAscii(self));
    }
    static InternalValue Isprintable(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "isprintable", 0, 0);
        auto chars = SplitCodePoints(self);
        return InternalValue(std::all_of(chars.begin(), chars.end(), [](View ch) { return detail::IsPythonPrintable(CodePointValue(ch)); }));
    }
    static InternalValue Isidentifier(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "isidentifier", 0, 0);
        auto chars = SplitCodePoints(self);
        if (chars.empty() || !unicode::IsXidStart(CodePointValue(chars.front())))
        {
            return InternalValue(false);
        }
        return InternalValue(std::all_of(std::next(chars.begin()), chars.end(), [](View ch) { return unicode::IsXidContinue(CodePointValue(ch)); }));
    }
    static InternalValue Isalpha(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isalpha", IsAlphaCp); }
    static InternalValue Isalnum(View self, const CallParams& params, RenderContext&)
    {
        return AllChars(self, params, "isalnum", [](uint32_t cp) { return unicode::IsWordChar(cp) && cp != '_'; });
    }
    static InternalValue Isspace(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isspace", unicode::IsSpace); }

    // CPython's unicode_isupper and unicode_islower (IsCaseOnly)
    static InternalValue CaseCheck(View self, const CallParams& params, const char* name, bool upper)
    {
        CheckArgs(params, name, 0, 0);
        return InternalValue(IsCaseOnly(self, upper));
    }
    static InternalValue Isupper(View self, const CallParams& params, RenderContext&) { return CaseCheck(self, params, "isupper", true); }
    static InternalValue Islower(View self, const CallParams& params, RenderContext&) { return CaseCheck(self, params, "islower", false); }
    // CPython's unicode_istitle: uppercase and titlecase characters only start a run of cased
    // characters, lowercase ones only continue one
    static InternalValue Istitle(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "istitle", 0, 0);
        bool hasCased = false;
        bool prevCased = false;
        for (auto ch : SplitCodePoints(self))
        {
            auto cp = CodePointValue(ch);
            if (unicode::IsUppercase(cp) || unicode::IsTitlecase(cp))
            {
                if (prevCased)
                {
                    return InternalValue(false);
                }
                prevCased = hasCased = true;
            }
            else if (unicode::IsLowercase(cp))
            {
                if (!prevCased)
                {
                    return InternalValue(false);
                }
                prevCased = hasCased = true;
            }
            else
            {
                prevCased = false;
            }
        }
        return InternalValue(hasCased);
    }

    static InternalValue Zfill(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "zfill", 1, 1);
        auto width = WidthArg(IntArg(params.posParams[0], "zfill"));
        auto len = static_cast<int64_t>(CodePointCount(self));
        if (width <= len)
        {
            return Result(Str(self.begin(), self.end()));
        }
        Str result(static_cast<size_t>(width - len), static_cast<CharT>('0'));
        if (!self.empty() && (self[0] == '+' || self[0] == '-'))
        {
            return Result(Str(1, self[0]) + result + Str(self.begin() + 1, self.end()));
        }
        return Result(result + Str(self.begin(), self.end()));
    }

    static InternalValue PadImpl(View self, const CallParams& params, const char* name, int align)
    {
        CheckArgs(params, name, 1, 2);
        auto width = WidthArg(IntArg(params.posParams[0], name));
        Str fill(1, static_cast<CharT>(' '));
        if (const auto* fillArg = Arg(params, 1))
        {
            fill = StrArg(self, *fillArg, name);
            if (CodePointCount(View(fill)) != 1)
            {
                Raise("The fill character must be exactly one character long"s);
            }
        }
        auto len = static_cast<int64_t>(CodePointCount(self));
        if (width <= len)
        {
            return Result(Str(self.begin(), self.end()));
        }
        auto pad = width - len;
        // Python's center() puts the odd character on the left when the width is odd
        int64_t left = (pad / 2) + (pad & width & 1);
        if (align != 0)
        {
            left = align < 0 ? 0 : pad;
        }
        Str result;
        for (int64_t n = 0; n < left; ++n)
        {
            result += fill;
        }
        result.append(self.begin(), self.end());
        for (int64_t n = left; n < pad; ++n)
        {
            result += fill;
        }
        return Result(std::move(result));
    }
    static InternalValue Center(View self, const CallParams& params, RenderContext&) { return PadImpl(self, params, "center", 0); }
    static InternalValue Ljust(View self, const CallParams& params, RenderContext&) { return PadImpl(self, params, "ljust", -1); }
    static InternalValue Rjust(View self, const CallParams& params, RenderContext&) { return PadImpl(self, params, "rjust", 1); }

    static InternalValue PartitionImpl(View self, const CallParams& params, const char* name, bool fromRight)
    {
        CheckArgs(params, name, 1, 1);
        auto sep = StrArg(self, params.posParams[0], name);
        if (sep.empty())
        {
            Raise("empty separator");
        }
        auto p = fromRight ? self.rfind(sep) : self.find(sep);
        Str whole(self.begin(), self.end());
        if (p == View::npos)
        {
            return fromRight ? MakeTuple({ TargetString(Str()), TargetString(Str()), TargetString(whole) })
                             : MakeTuple({ TargetString(whole), TargetString(Str()), TargetString(Str()) });
        }
        return MakeTuple({ TargetString(Str(self.substr(0, p))), TargetString(sep), TargetString(Str(self.substr(p + sep.size()))) });
    }
    static InternalValue Partition(View self, const CallParams& params, RenderContext&) { return PartitionImpl(self, params, "partition", false); }
    static InternalValue Rpartition(View self, const CallParams& params, RenderContext&) { return PartitionImpl(self, params, "rpartition", true); }

    static InternalValue Removeprefix(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "removeprefix", 1, 1);
        auto prefix = StrArg(self, params.posParams[0], "removeprefix");
        if (self.substr(0, prefix.size()) == View(prefix))
        {
            self.remove_prefix(prefix.size());
        }
        return Result(Str(self.begin(), self.end()));
    }
    static InternalValue Removesuffix(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "removesuffix", 1, 1);
        auto suffix = StrArg(self, params.posParams[0], "removesuffix");
        if (!suffix.empty() && self.size() >= suffix.size() && self.substr(self.size() - suffix.size()) == View(suffix))
        {
            self.remove_suffix(suffix.size());
        }
        return Result(Str(self.begin(), self.end()));
    }

    // Python's expandtabs: a tab pads to the next multiple of tabsize columns, \n and \r start
    // column 0 again, a tabsize of 0 or less drops tabs; columns count code points. The size is
    // checked against the sequence cap before the string is built (docs/tasks/0452)
    static InternalValue Expandtabs(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "expandtabs", 0, 1, { "tabsize" });
        int64_t tabsize = 8;
        if (const auto* arg = Arg(params, 0, "tabsize"))
        {
            tabsize = IntArg(*arg, "expandtabs");
            // Python's tabsize is a C int
            if (tabsize < std::numeric_limits<int>::min() || tabsize > std::numeric_limits<int>::max())
            {
                Raise("OverflowError: Python int too large to convert to C int"s);
            }
        }
        uint64_t size = 0;
        auto forEachPiece = [&self, tabsize](const auto& fn) {
            int64_t column = 0;
            for (auto ch : self)
            {
                if (ch == '\t')
                {
                    if (tabsize > 0)
                    {
                        const auto pad = tabsize - (column % tabsize);
                        column += pad;
                        fn(ch, pad);
                    }
                    continue;
                }
                fn(ch, 0);
                if (ch == '\n' || ch == '\r')
                {
                    column = 0;
                }
                else if (!IsCodePointTail(ch))
                {
                    ++column;
                }
            }
        };
        // Each step stays within the cap, so neither the size nor the column overflows
        forEachPiece([&size](CharT, int64_t pad) {
            size += pad > 0 ? static_cast<uint64_t>(pad) : 1;
            CheckSequenceSize(size, "an expanded string");
        });
        Str result;
        result.reserve(static_cast<size_t>(size));
        forEachPiece([&result](CharT ch, int64_t pad) {
            if (pad > 0)
            {
                result.append(static_cast<size_t>(pad), static_cast<CharT>(' '));
            }
            else
            {
                result.push_back(ch);
            }
        });
        return Result(std::move(result));
    }

    // table[cp] as str.translate looks it up: nullopt where the table has no such item (Python's
    // LookupError, which keeps the character)
    static std::optional<InternalValue> TranslateLookup(const InternalValue& table, uint32_t cp, RenderContext& context)
    {
        const InternalValue key(static_cast<int64_t>(cp));
        if (table.IsUndefined())
        {
            CheckUndefinedUse(table, UndefinedUse::Attribute);
            return std::nullopt;
        }
        if (const auto* host = GetHostObject(table))
        {
            return host->GetItem(key);
        }
        if (const auto map = AsMap(table); map && map->GetAttrPolicy() != MapAttrPolicy::KeysOnly)
        {
            return map->FindItem(key);
        }
        const auto list = AsList(table);
        if (list && table.Kind() == ValueKind::List && list->GetIteratorKind() == IteratorKind::None)
        {
            const auto size = list->GetSize();
            return size && cp < *size ? std::optional<InternalValue>(Subscript(table, key, &context)) : std::nullopt;
        }
        Raise("'" + TypeName(table) + "' object is not subscriptable");
    }

    // Python's str.translate(table): each character's table item is None (dropped), an int (a code
    // point) or a str; a character the table lacks stays
    static InternalValue Translate(View self, const CallParams& params, RenderContext& context)
    {
        CheckArgs(params, "translate", 1, 1);
        const auto& table = params.posParams[0];
        Str result;
        // Each piece is checked against the sequence cap before it is appended, so a large
        // replacement never builds an oversized result first
        auto append = [&result](View piece) {
            CheckSequenceSize(static_cast<uint64_t>(result.size()) + piece.size(), "a translated string");
            result.append(piece);
        };
        // A str table is split once: indexing it per character would rescan it each time
        if (IsStringValue(table))
        {
            const auto tableStr = StrArg(self, table, "translate");
            const auto tableChars = SplitCodePoints(View(tableStr));
            ForEachCodePoint(self, [&append, &tableChars](View ch) {
                const auto cp = CodePointValue(ch);
                append(cp < tableChars.size() ? tableChars[cp] : ch);
            });
            return Result(std::move(result));
        }
        ForEachCodePoint(self, [&result, &append, &table, &self, &context](View ch) {
            const auto mapped = TranslateLookup(table, CodePointValue(ch), context);
            if (!mapped)
            {
                append(ch);
            }
            else if (IsStringValue(*mapped))
            {
                append(StrArg(self, *mapped, "translate"));
            }
            else if (mapped->Kind() == ValueKind::Int || mapped->Kind() == ValueKind::Bool || mapped->Kind() == ValueKind::BigInt)
            {
                const auto* value = GetIf<int64_t>(&*mapped);
                const auto cp = value ? *value : (mapped->Kind() == ValueKind::Bool ? int64_t{ *GetIf<bool>(&*mapped) } : -1);
                if (cp < 0 || cp > 0x10FFFF)
                {
                    Raise("character mapping must be in range(0x110000)"s);
                }
                CheckSequenceSize(static_cast<uint64_t>(result.size()) + EncodedUnits<CharT>(static_cast<uint32_t>(cp)), "a translated string");
                AppendCodePoint(result, static_cast<uint32_t>(cp));
            }
            else if (!mapped->IsNone())
            {
                Raise("character mapping must return integer, None or str"s);
            }
        });
        return Result(std::move(result));
    }

    // The one code point of a str key of maketrans's dict, or of a character of its arguments
    static uint32_t SingleCodePoint(View str) { return CodePointValue(str); }

    static Str MaketransStr(View self, const CallParams& params, size_t idx)
    {
        const auto& val = params.posParams[idx];
        auto str = GetAsSameString(self, val);
        if (!str || !IsStringValue(val))
        {
            Raise(fmt::format("maketrans() argument {} must be str, not {}", idx + 1, TypeName(val)));
        }
        return std::move(*str);
    }

    // Python's str.maketrans: a dict of code points to what translate puts in their place, from
    // one dict (its str keys of one character become code points) or from two strings of the
    // same length and a third whose characters are dropped
    static InternalValue Maketrans(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "maketrans", 1, 3);
        TemplateDict result;
        const auto& first = params.posParams[0];
        if (params.posParams.size() == 1)
        {
            const auto map = AsMap(first);
            if (!map || first.Kind() != ValueKind::Map || map->GetAttrPolicy() == MapAttrPolicy::KeysOnly)
            {
                Raise("if you give only one argument to maketrans it must be a dict"s);
            }
            for (auto& entry : map->GetEntries())
            {
                if (IsStringValue(entry.key))
                {
                    const auto key = StrArg(self, entry.key, "maketrans");
                    if (CodePointCount(View(key)) != 1)
                    {
                        Raise("string keys in translate table must be of length 1"s);
                    }
                    result.Set(InternalValue(static_cast<int64_t>(SingleCodePoint(key))), std::move(entry.value));
                }
                else if (const auto kind = entry.key.Kind(); kind == ValueKind::Int || kind == ValueKind::Bool || kind == ValueKind::BigInt)
                {
                    result.Set(entry.key, std::move(entry.value));
                }
                else
                {
                    Raise("keys in translate table must be strings or integers"s);
                }
            }
            return CreateMapAdapter(std::move(result));
        }
        if (!IsStringValue(first))
        {
            Raise("first maketrans argument must be a string if there is a second argument"s);
        }
        const auto from = MaketransStr(self, params, 0);
        const auto to = MaketransStr(self, params, 1);
        const auto fromChars = SplitCodePoints(View(from));
        const auto toChars = SplitCodePoints(View(to));
        if (fromChars.size() != toChars.size())
        {
            Raise("the first two maketrans arguments must have equal length"s);
        }
        for (size_t n = 0; n < fromChars.size(); ++n)
        {
            result.Set(InternalValue(static_cast<int64_t>(SingleCodePoint(fromChars[n]))), InternalValue(static_cast<int64_t>(SingleCodePoint(toChars[n]))));
        }
        if (params.posParams.size() == 3)
        {
            const auto dropped = MaketransStr(self, params, 2);
            ForEachCodePoint(View(dropped), [&result](View ch) { result.Set(InternalValue(static_cast<int64_t>(SingleCodePoint(ch))), InternalValue(NoneValue())); });
        }
        return CreateMapAdapter(std::move(result));
    }

    static Str ToStr(const InternalValue& val, bool asRepr)
    {
        Str result;
        visitors::RenderValue(val, result, asRepr);
        return result;
    }

    static Str Ascii(const std::string& str) { return Str(str.begin(), str.end()); }

    // Python's ascii(): the repr with every non-ASCII character escaped as \\xhh, \\uhhhh or
    // \\Uhhhhhhhh
    static Str AsciiRepr(const InternalValue& val)
    {
        auto repr = ToStr(val, true);
        if (IsAscii(repr))
        {
            return repr;
        }
        Str result;
        for (auto ch : SplitCodePoints(View(repr)))
        {
            const auto cp = CodePointValue(ch);
            if (cp < 0x80)
            {
                CheckSequenceSize(static_cast<uint64_t>(result.size()) + ch.size(), "a string");
                result.append(ch.begin(), ch.end());
                continue;
            }
            std::string escaped;
            if (cp <= 0xFF)
            {
                escaped = fmt::format("\\x{:02x}", cp);
            }
            else if (cp <= 0xFFFF)
            {
                escaped = fmt::format("\\u{:04x}", cp);
            }
            else
            {
                escaped = fmt::format("\\U{:08x}", cp);
            }
            CheckSequenceSize(static_cast<uint64_t>(result.size()) + escaped.size(), "a string");
            result.append(escaped.begin(), escaped.end());
        }
        return result;
    }

    // A parsed spec in the subset of Python's format mini-language that templates use:
    // [[fill]align][sign][#][0][width][,|_][.precision][type]
    struct FormatSpec
    {
        Str fill = Str(1, static_cast<CharT>(' '));
        CharT align = 0;
        uint32_t sign = 0;
        bool alternate = false;
        int64_t width = 0;
        uint32_t grouping = 0;
        int64_t precision = -1;
        uint32_t type = 0;
    };

    static bool IsAlign(uint32_t cp) { return cp == '<' || cp == '>' || cp == '^' || cp == '='; }

    static FormatSpec ParseFormatSpec(View text)
    {
        FormatSpec spec;
        auto chars = SplitCodePoints(text);
        size_t pos = 0;
        if (chars.size() >= 2 && IsAlign(CodePointValue(chars[1])))
        {
            spec.fill = Str(chars[0].begin(), chars[0].end());
            spec.align = chars[1][0];
            pos = 2;
        }
        else if (!chars.empty() && IsAlign(CodePointValue(chars[0])))
        {
            spec.align = chars[0][0];
            pos = 1;
        }
        auto peek = [&]() -> uint32_t { return pos < chars.size() ? CodePointValue(chars[pos]) : 0; };
        if (peek() == '+' || peek() == '-' || peek() == ' ')
        {
            spec.sign = CodePointValue(chars[pos++]);
        }
        if (peek() == '#')
        {
            spec.alternate = true;
            ++pos;
        }
        if (peek() == '0')
        {
            if (spec.align == 0)
            {
                spec.fill = Str(1, static_cast<CharT>('0'));
                spec.align = '=';
            }
            ++pos;
        }
        auto readNumber = [&]() {
            int64_t value = 0;
            while (peek() >= '0' && peek() <= '9')
            {
                value = WidthArg((value * 10) + (CodePointValue(chars[pos++]) - '0'));
            }
            return value;
        };
        spec.width = readNumber();
        if (peek() == ',' || peek() == '_')
        {
            spec.grouping = CodePointValue(chars[pos++]);
        }
        if (peek() == '.')
        {
            ++pos;
            if (!(peek() >= '0' && peek() <= '9'))
            {
                Raise("Format specifier missing precision");
            }
            spec.precision = readNumber();
        }
        if (pos < chars.size())
        {
            spec.type = CodePointValue(chars[pos++]);
        }
        if (pos != chars.size())
        {
            Raise("Invalid format specifier");
        }
        return spec;
    }

    static Str FormatAsString(const InternalValue& val, const FormatSpec& spec)
    {
        if (spec.type != 0 && spec.type != 's')
        {
            Raise(fmt::format("Unknown format code '{}' for object of type '{}'", static_cast<char>(spec.type), TypeName(val)));
        }
        if (spec.sign != 0 || spec.alternate || spec.grouping != 0 || spec.align == '=')
        {
            Raise("Invalid format specifier for a string");
        }
        Str body = ToStr(val, false);
        if (spec.precision >= 0)
        {
            auto bodyChars = SplitCodePoints(View(body));
            if (static_cast<size_t>(spec.precision) < bodyChars.size())
            {
                body = Join(bodyChars, 0, static_cast<size_t>(spec.precision));
            }
        }
        return body;
    }

    static bool IsIntegerType(uint32_t type)
    {
        return type == 0 || type == 'd' || type == 'b' || type == 'o' || type == 'x' || type == 'X' || type == 'n' || type == 'c';
    }

    static bool IsFloatType(uint32_t type)
    {
        return type == 0 || type == 'e' || type == 'E' || type == 'f' || type == 'F' || type == 'g' || type == 'G' || type == '%' || type == 'n';
    }

    static int IntegerBase(uint32_t type)
    {
        if (type == 'b')
        {
            return 2;
        }
        if (type == 'o')
        {
            return 8;
        }
        return type == 'x' || type == 'X' ? 16 : 10;
    }

    // Digits of magnitude in the base the type selects, grouped and with the 0b/0o/0x prefix
    static std::string IntegerDigits(uint64_t magnitude, const FormatSpec& spec)
    {
        const auto type = spec.type;
        const int base = IntegerBase(type);
        std::string digits;
        do
        {
            auto d = static_cast<int>(magnitude % static_cast<uint64_t>(base));
            digits.push_back(static_cast<char>(d < 10 ? '0' + d : (type == 'X' ? 'A' : 'a') + d - 10));
            magnitude /= static_cast<uint64_t>(base);
        } while (magnitude != 0);
        return DecorateDigits(std::move(digits), base, spec);
    }

    // The same for an integer outside int64 (docs/tasks/0270): decimal digits are subject to
    // CPython's digit limit, the power-of-two bases are not
    static std::string IntegerDigits(const BigIntView& val, const FormatSpec& spec)
    {
        const int base = IntegerBase(spec.type);
        std::string digits;
        if (base == 10)
        {
            CheckedDecimal(val);
            digits = std::string(val.Magnitude());
        }
        else
        {
            digits = MagnitudeInBase(val, base);
            if (spec.type == 'X')
            {
                std::transform(digits.begin(), digits.end(), digits.begin(), [](char ch) { return ch >= 'a' && ch <= 'f' ? static_cast<char>(ch - 'a' + 'A') : ch; });
            }
        }
        std::reverse(digits.begin(), digits.end());
        return DecorateDigits(std::move(digits), base, spec);
    }

    // Groups digits, given least significant first, and adds the prefix
    static std::string DecorateDigits(std::string digits, int base, const FormatSpec& spec)
    {
        const auto type = spec.type;
        if (spec.grouping != 0)
        {
            size_t groupSize = base == 10 ? 3 : 4;
            std::string grouped;
            for (size_t n = 0; n < digits.size(); ++n)
            {
                if (n != 0 && n % groupSize == 0)
                {
                    grouped.push_back(static_cast<char>(spec.grouping));
                }
                grouped.push_back(digits[n]);
            }
            digits = grouped;
        }
        std::reverse(digits.begin(), digits.end());
        if (spec.alternate && base != 10)
        {
            digits = std::string(1, '0') + static_cast<char>(type) + digits;
        }
        return digits;
    }

    // Digits of a non-negative double for the float presentation types
    static std::string FloatDigits(double magnitude, const FormatSpec& spec)
    {
        const auto type = spec.type;
        int prec = spec.precision < 0 ? 6 : static_cast<int>(spec.precision);
        if (std::isinf(magnitude) || std::isnan(magnitude))
        {
            return std::isnan(magnitude) ? "nan" : "inf";
        }
        if (type == 'f' || type == 'F')
        {
            return fmt::format("{:.{}f}", magnitude, prec);
        }
        if (type == '%')
        {
            return fmt::format("{:.{}f}", magnitude * 100, prec) + "%";
        }
        if (type == 'e' || type == 'E')
        {
            return fmt::format("{:.{}e}", magnitude, prec);
        }
        if (type == 0 && spec.precision < 0)
        {
            return visitors::FormatPythonFloat(magnitude);
        }
        // 'g' (and no type with a precision): significant digits, trailing zeros dropped
        auto digits = fmt::format("{:.{}g}", magnitude, std::max(prec, 1));
        if (type == 0 && digits.find_first_of(".e") == std::string::npos)
        {
            digits += ".0";
        }
        return digits;
    }

    // Inserts the separator every three digits of the integer part
    static std::string GroupIntegerPart(const std::string& digits, uint32_t separator)
    {
        auto intEnd = digits.find_first_not_of("0123456789");
        if (intEnd == std::string::npos)
        {
            intEnd = digits.size();
        }
        std::string grouped;
        for (size_t n = 0; n < intEnd; ++n)
        {
            if (n != 0 && (intEnd - n) % 3 == 0)
            {
                grouped.push_back(static_cast<char>(separator));
            }
            grouped.push_back(digits[n]);
        }
        return grouped + digits.substr(intEnd);
    }

    static std::string FormatFloat(double magnitude, const FormatSpec& spec)
    {
        auto digits = FloatDigits(magnitude, spec);
        if (spec.type == 'E' || spec.type == 'F' || spec.type == 'G')
        {
            std::transform(digits.begin(), digits.end(), digits.begin(), [](char ch) { return static_cast<char>(std::toupper(static_cast<unsigned char>(ch))); });
        }
        if (spec.grouping != 0)
        {
            digits = GroupIntegerPart(digits, spec.grouping);
        }
        return digits;
    }

    // Pads sign and body to the spec's width; '=' puts the padding between them
    static Str PadFormatted(const FormatSpec& spec, const Str& signStr, const Str& body, bool numeric)
    {
        auto len = static_cast<int64_t>(CodePointCount(View(signStr)) + CodePointCount(View(body)));
        if (spec.width <= len)
        {
            return signStr + body;
        }
        auto pad = spec.width - len;
        CharT align = spec.align;
        if (align == 0)
        {
            align = numeric ? '>' : '<';
        }
        auto makePad = [&spec](int64_t n) {
            Str result;
            for (int64_t i = 0; i < n; ++i)
            {
                result += spec.fill;
            }
            return result;
        };
        switch (align)
        {
        case '<':
            return signStr + body + makePad(pad);
        case '^':
            return makePad(pad / 2) + signStr + body + makePad(pad - (pad / 2));
        case '=':
            return signStr + makePad(pad) + body;
        default:
            return makePad(pad) + signStr + body;
        }
    }

    // A formatted value before padding: the sign is added by PadFormatted
    struct Formatted
    {
        Str body;
        bool numeric = false;
        bool negative = false;
    };

    static Formatted FormatInteger(int64_t value, const FormatSpec& spec)
    {
        if (spec.precision >= 0)
        {
            Raise("Precision not allowed in integer format specifier");
        }
        Formatted result;
        if (spec.type == 'c')
        {
            result.body = ToStr(InternalValue(TargetString(Str(1, static_cast<CharT>(value)))), false);
            return result;
        }
        result.numeric = true;
        result.negative = value < 0;
        uint64_t magnitude = result.negative ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
        result.body = Ascii(IntegerDigits(magnitude, spec));
        return result;
    }

    static Formatted FormatBigInteger(const BigIntView& value, const FormatSpec& spec)
    {
        if (spec.precision >= 0)
        {
            Raise("Precision not allowed in integer format specifier");
        }
        if (spec.type == 'c')
        {
            throw IntegerOutOfRangeError("%c arg not in range(0x110000)");
        }
        Formatted result;
        result.numeric = true;
        result.negative = value.Sign() < 0;
        result.body = Ascii(IntegerDigits(value, spec));
        return result;
    }

    static Formatted FormatReal(const InternalValue& val, double value, const FormatSpec& spec)
    {
        if (!IsFloatType(spec.type))
        {
            Raise(fmt::format("Unknown format code '{}' for object of type '{}'", static_cast<char>(spec.type), TypeName(val)));
        }
        Formatted result;
        result.numeric = true;
        result.negative = std::signbit(value) && !std::isnan(value);
        result.body = Ascii(FormatFloat(std::fabs(value), spec));
        return result;
    }

    // format(value, spec), see FormatSpec for the supported spec
    static Str FormatValue(const InternalValue& val, View specText)
    {
        if (specText.empty())
        {
            return ToStr(val, false);
        }
        auto spec = ParseFormatSpec(specText);
        const auto type = spec.type;

        const auto* intVal = GetIf<int64_t>(&val);
        const auto* boolVal = GetIf<bool>(&val);
        const auto* dblVal = GetIf<double>(&val);
        const auto bigVal = AsBigInt(val);
        int64_t intValue = 0;
        if (intVal)
        {
            intValue = *intVal;
        }
        else if (boolVal)
        {
            intValue = *boolVal ? 1 : 0;
        }
        const bool isInt = intVal != nullptr || bigVal.has_value() || (boolVal != nullptr && type != 0 && type != 's');
        Formatted out;
        if (IsStringValue(val) || (boolVal && type == 0))
        {
            out.body = FormatAsString(val, spec);
        }
        else if (isInt && IsIntegerType(type))
        {
            out = bigVal ? FormatBigInteger(*bigVal, spec) : FormatInteger(intValue, spec);
        }
        else if (isInt || dblVal)
        {
            auto real = static_cast<double>(intValue);
            if (dblVal)
            {
                real = *dblVal;
            }
            else if (bigVal)
            {
                real = BigIntToDouble(*bigVal);
            }
            out = FormatReal(val, real, spec);
        }
        else
        {
            Raise("unsupported format string passed to " + TypeName(val) + ".__format__");
        }

        Str signStr;
        if (out.negative)
        {
            signStr = Ascii("-");
        }
        else if (out.numeric && (spec.sign == '+' || spec.sign == ' '))
        {
            signStr = Str(1, static_cast<CharT>(spec.sign));
        }
        return PadFormatted(spec, signStr, out.body, out.numeric);
    }

    static std::string Narrow(View str) { return ConvertString<std::string>(str); }

    static bool IsDigits(View str)
    {
        return !str.empty() && std::all_of(str.begin(), str.end(), [](CharT c) { return c >= '0' && c <= '9'; });
    }

    // A field's index or key of digits: Python's limit is its Py_ssize_t
    static std::int64_t ParseFieldNumber(View digits)
    {
        const auto narrow = Narrow(digits);
        std::int64_t result = 0;
        const auto parsed = std::from_chars(narrow.data(), narrow.data() + narrow.size(), result);
        if (parsed.ec != std::errc())
        {
            Raise("Too many decimal digits in format string");
        }
        return result;
    }

    // Python forbids mixing {} with {0} in one format string. mapping: format_map's argument,
    // the only source of named fields, which leaves no positional ones
    struct FieldNumbering
    {
        size_t autoIdx = 0;
        bool usedAuto = false;
        bool usedManual = false;
        const InternalValue* mapping = nullptr;
    };

    // The argument a field's first name refers to: a position ({} or {0}) or a keyword, or for
    // format_map the mapping's item (CPython's get_field_object)
    static InternalValue LookupArg(View first, const CallParams& params, FieldNumbering& numbering, RenderContext& context)
    {
        if (!first.empty() && !IsDigits(first))
        {
            if (numbering.mapping)
            {
                return FormatItem(*numbering.mapping, InternalValue(Narrow(first)), context);
            }
            auto p = params.kwParams.find(Narrow(first));
            if (p == params.kwParams.end())
            {
                Raise("'" + Narrow(first) + "'");
            }
            return p->second;
        }
        size_t idx = 0;
        if (first.empty())
        {
            if (numbering.usedManual)
            {
                Raise("cannot switch from manual field specification to automatic field numbering");
            }
            numbering.usedAuto = true;
            idx = numbering.autoIdx++;
        }
        else
        {
            if (numbering.usedAuto)
            {
                Raise("cannot switch from automatic field numbering to manual field specification");
            }
            numbering.usedManual = true;
            idx = static_cast<size_t>(ParseFieldNumber(first));
        }
        if (numbering.mapping)
        {
            Raise("Format string contains positional fields");
        }
        if (idx >= params.posParams.size())
        {
            Raise(fmt::format("Replacement index {} out of range for positional args tuple", idx));
        }
        return params.posParams[idx];
    }

    // Python's getattr, as a field's .attr part looks it up: the methods of a value and the
    // attributes of an object, but not the items of a dict (the attr filter's rule)
    static InternalValue FormatAttr(const InternalValue& value, const std::string& attr, RenderContext& context)
    {
        if (value.IsUndefined())
        {
            CheckUndefinedUse(value, UndefinedUse::Attribute);
            return value;
        }
        std::optional<InternalValue> result;
        auto map = AsMap(value);
        const bool isObject = map && (map->HasAttributes() || map->GetObjectClass() != nullptr);
        // A named tuple (groupby's groups) has its fields as attributes
        const auto list = AsList(value);
        const auto* fields = list ? list->GetFieldNames() : nullptr;
        const bool isField = fields != nullptr && std::find(fields->begin(), fields->end(), attr) != fields->end();
        if (const auto* host = GetHostObject(value))
        {
            result = host->GetAttr(attr);
        }
        else if (auto replacement = SandboxedNativeAttribute(&context, value, attr))
        {
            // The sandbox's formatter reads a field's attribute as the sandbox lets the template read it: a refused
            // one is its undefined value, which formats as one (SandboxedFormatter.get_field)
            return std::move(*replacement);
        }
        else if (const auto* method = FindMethod(value, attr); method && (!isObject || map->GetAttrPolicy() == MapAttrPolicy::MethodsFirst || !map->HasValue(attr)))
        {
            result = MakeBoundMethod(value, *method);
        }
        else if (auto attrValue = GetNumberDataAttr(value, attr, nullptr)) // the policy was asked above
        {
            result = std::move(attrValue);
        }
        else if (auto cls = ClassAttribute(value, attr, nullptr))
        {
            result = std::move(cls);
        }
        else if (isObject || isField || value.Kind() == ValueKind::Callable)
        {
            // The fields of a reflected object, of namespace(), loop or a named tuple, the
            // attributes of a macro
            result = Subscript(value, attr, &context);
        }
        // An attribute that holds an undefined value is there and formats as one
        const bool present = isField || (isObject && map->HasValue(attr));
        if (!result || (result->IsUndefined() && !present))
        {
            Raise("'" + TypeName(value) + "' object has no attribute '" + attr + "'");
        }
        return std::move(*result);
    }

    // Whether the key or index is in a map or list, so that an item that holds an undefined
    // value is told from a missing one
    static bool HasItem(const InternalValue& value, const InternalValue& key)
    {
        if (auto map = AsMap(value))
        {
            return IsStringValue(key) && map->HasValue(AsString(key));
        }
        const auto list = AsList(value);
        const auto* idx = GetIf<int64_t>(&key);
        const auto size = list ? list->GetSize() : std::nullopt;
        if (!idx || !size)
        {
            return false;
        }
        const auto count = static_cast<int64_t>(*size);
        return *idx >= -count && *idx < count;
    }

    // A str key of a str, list, tuple or range: Python's TypeError
    static void ThrowIfSequence(const InternalValue& value)
    {
        if (IsStringValue(value))
        {
            Raise("string indices must be integers, not 'str'");
        }
        const auto list = AsList(value);
        if ((list && list->GetIteratorKind() == IteratorKind::None) || value.Kind() == ValueKind::KeyValuePair)
        {
            // A named tuple (groupby's groups) is a tuple
            Raise((list && list->GetFieldNames() ? std::string("tuple") : TypeName(value)) + " indices must be integers or slices, not str");
        }
    }

    // Python's obj[key], as a field's [key] part looks it up: __getitem__ only, so an object
    // such as namespace() or loop is not subscriptable and a missing key fails
    static InternalValue FormatItem(const InternalValue& value, const InternalValue& key, RenderContext& context)
    {
        if (value.IsUndefined())
        {
            CheckUndefinedUse(value, UndefinedUse::Attribute);
            return value;
        }
        std::optional<InternalValue> result;
        auto map = AsMap(value);
        if (const auto* host = GetHostObject(value))
        {
            result = host->GetItem(key);
        }
        // Python's str.format reads obj[key], which a number has not; only the sandbox's formatter goes through
        // getitem, which falls back to the number's data attributes (GetItem)
        else if (const auto kind = value.Kind(); kind == ValueKind::Callable || (map && map->GetAttrPolicy() == MapAttrPolicy::KeysOnly)
                                                 || (!context.GetSandboxPolicy() && (kind == ValueKind::Int || kind == ValueKind::BigInt || kind == ValueKind::Double || kind == ValueKind::Bool)))
        {
            Raise("'" + TypeName(value) + "' object is not subscriptable");
        }
        // A dict's items only: GetItem would fall back to its methods ('{0[items]}' is a KeyError)
        else if (map && value.Kind() == ValueKind::Map)
        {
            result = map->FindItem(key);
        }
        else
        {
            // A sequence takes an index only: GetItem would fall back to its attributes ('{0[start]}' of a range, a
            // named tuple's field), which only the sandbox's formatter reaches through getitem (docs/tasks/0475)
            const bool plainFormatter = IsStringValue(key) && context.GetSandboxPolicy() == nullptr;
            if (plainFormatter)
            {
                ThrowIfSequence(value);
            }
            // Nor is __class__ an item of anything that has items ('{0[__class__]}' of None fails)
            result = plainFormatter && AsString(key) == "__class__" ? Subscript(value, key, &context) : GetItem(value, key, &context);
        }
        if (!result || (result->IsUndefined() && !HasItem(value, key)))
        {
            if (map)
            {
                Raise("KeyError: " + Narrow(View(ToStr(key, true))));
            }
            if (value.Kind() == ValueKind::List || value.Kind() == ValueKind::String)
            {
                Raise(TypeName(value) + " index out of range");
            }
            Raise("'" + TypeName(value) + "' object is not subscriptable");
        }
        return std::move(*result);
    }

    // Applies the .attr and [key] parts that follow a field's first name
    static InternalValue ApplyAccessors(InternalValue value, View rest, RenderContext& context)
    {
        while (!rest.empty())
        {
            if (rest[0] == '.')
            {
                auto end = rest.find_first_of(Ascii(".["), 1);
                auto attr = Narrow(rest.substr(1, end == View::npos ? View::npos : end - 1));
                if (attr.empty())
                {
                    Raise("Empty attribute in format string");
                }
                value = FormatAttr(value, attr, context);
                rest = end == View::npos ? View() : rest.substr(end);
                continue;
            }
            auto end = rest.find(']');
            if (end == View::npos)
            {
                Raise("Missing ']' in format string");
            }
            auto key = rest.substr(1, end - 1);
            InternalValue keyVal = IsDigits(key) ? InternalValue(ParseFieldNumber(key)) : InternalValue(Narrow(key));
            value = FormatItem(value, keyVal, context);
            rest = rest.substr(end + 1);
            if (!rest.empty() && rest[0] != '.' && rest[0] != '[')
            {
                Raise("Only '.' or '[' may follow ']' in format field specifier");
            }
        }
        return value;
    }

    // A replacement field, parsed: name[!conversion][:spec]
    struct Field
    {
        View name;
        CharT conversion = 0;
        View spec;
        // The spec holds fields of its own ('{:{}}')
        bool nestedSpec = false;
    };

    // CPython's parse_field on the text after a field's '{'; pos ends after its '}'. A '}' or
    // ':' inside [key] belongs to the key.
    static Field ParseField(View str, size_t& pos)
    {
        Field field;
        const auto nameStart = pos;
        CharT stop = 0;
        while (pos < str.size())
        {
            auto ch = str[pos++];
            if (ch == '{')
            {
                Raise("unexpected '{' in field name");
            }
            if (ch == '[')
            {
                while (pos < str.size() && str[pos] != ']')
                {
                    ++pos;
                }
            }
            else if (ch == '}' || ch == ':' || ch == '!')
            {
                stop = ch;
                break;
            }
        }
        if (stop == 0)
        {
            Raise("expected '}' before end of string");
        }
        field.name = str.substr(nameStart, pos - 1 - nameStart);
        if (stop == '!' && !ParseConversion(str, pos, field))
        {
            return field;
        }
        if (stop == '!' || stop == ':')
        {
            ParseSpec(str, pos, field);
        }
        return field;
    }

    // The conversion after '!'; false when the field ends right after it ('{!r}')
    static bool ParseConversion(View str, size_t& pos, Field& field)
    {
        if (pos >= str.size())
        {
            Raise("end of string while looking for conversion specifier");
        }
        field.conversion = str[pos++];
        if (pos < str.size())
        {
            auto ch = str[pos++];
            if (ch == '}')
            {
                return false;
            }
            if (ch != ':')
            {
                Raise("expected ':' after conversion specifier");
            }
        }
        return true;
    }

    // The spec after ':', which ends at the '}' that balances the field's '{'
    static void ParseSpec(View str, size_t& pos, Field& field)
    {
        const auto specStart = pos;
        size_t depth = 1;
        while (pos < str.size())
        {
            auto ch = str[pos++];
            if (ch == '{')
            {
                field.nestedSpec = true;
                ++depth;
            }
            else if (ch == '}' && --depth == 0)
            {
                field.spec = str.substr(specStart, pos - 1 - specStart);
                return;
            }
        }
        Raise("unmatched '{' in format spec");
    }

    // One replacement field. Its value is looked up first, then the nested fields of its spec
    // are expanded, so '{:{}}' numbers the value 0 and the width 1.
    // escape: Markup's format() (markupsafe's EscapeFormatter), which escapes each field after
    // formatting it and takes Markup fields as they are; nested fields are escaped too
    static Str ReplaceField(const Field& field, const CallParams& params, FieldNumbering& numbering, RenderContext& context, bool escape, int depth)
    {
        // The first name, then .attr and [key] parts
        auto nameEnd = field.name.find_first_of(Ascii(".["));
        auto first = field.name.substr(0, nameEnd == View::npos ? field.name.size() : nameEnd);
        auto rest = nameEnd == View::npos ? View() : field.name.substr(nameEnd);
        auto value = ApplyAccessors(LookupArg(first, params, numbering, context), rest, context);
        if (field.conversion == 'a')
        {
            value = TargetString(AsciiRepr(value));
        }
        else if (field.conversion == 'r' || field.conversion == 's')
        {
            value = TargetString(ToStr(value, field.conversion == 'r'));
        }
        else if (field.conversion != 0)
        {
            Raise("Unknown conversion specifier");
        }
        Str expanded;
        View spec = field.spec;
        if (field.nestedSpec)
        {
            expanded = Build(field.spec, params, numbering, context, escape, depth - 1);
            spec = expanded;
        }
        if (!escape)
        {
            return FormatValue(value, spec);
        }
        if (value.IsMarkup())
        {
            if (!spec.empty())
            {
                Raise("Unsupported format specification for Markup.");
            }
            return ToStr(value, false);
        }
        return EscapeText<CharT>(FormatValue(value, spec), context.GetRendererCallback());
    }

    // CPython's build_string: the literal text with {{ and }} unescaped and each replacement
    // field formatted; depth counts down to the nested fields of a spec, which may not nest
    // again (Max string recursion exceeded)
    static Str Build(View self, const CallParams& params, FieldNumbering& numbering, RenderContext& context, bool escape, int depth)
    {
        if (depth <= 0)
        {
            Raise("Max string recursion exceeded");
        }
        Str result;
        for (size_t pos = 0; pos < self.size();)
        {
            auto ch = self[pos++];
            if (ch != '{' && ch != '}')
            {
                result.push_back(ch);
                continue;
            }
            const bool doubled = pos < self.size() && self[pos] == ch;
            if (doubled)
            {
                result.push_back(ch);
                ++pos;
                continue;
            }
            if (ch == '}')
            {
                Raise("Single '}' encountered in format string");
            }
            if (pos == self.size())
            {
                Raise("Single '{' encountered in format string");
            }
            auto field = ParseField(self, pos);
            result += ReplaceField(field, params, numbering, context, escape, depth);
            CheckSequenceSize(result.size(), "a formatted string");
        }
        return result;
    }

    // str.format: {}, {0}, {name}, attribute and index lookups {0.x} {a[k]}, !r/!s/!a, a
    // format spec and fields nested in it ('{:{}.{}f}')
    static InternalValue Format(View self, const CallParams& params, RenderContext& context, bool escape = false)
    {
        FieldNumbering numbering;
        return Result(Build(self, params, numbering, context, escape, 2));
    }

    // str.format_map(mapping): str.format with the mapping's items as the named fields and no
    // positional ones (docs/tasks/0443)
    static InternalValue FormatMap(View self, const CallParams& params, RenderContext& context, bool escape = false)
    {
        CheckArgs(params, "format_map", 1, 1);
        FieldNumbering numbering;
        numbering.mapping = params.posParams.data();
        return Result(Build(self, params, numbering, context, escape, 2));
    }
};

#define JINJA2CPP_STR_METHOD(Fn) \
    InternalValue Str##Fn(const InternalValue& self, const CallParams& params, RenderContext& context) \
    { \
        return ApplyStringConverter(self, [&params, &context](auto sv) -> InternalValue { \
            using CharT = typename decltype(sv)::value_type; \
            return StrOps<CharT>::Fn(sv, params, context); \
        }); \
    }

JINJA2CPP_STR_METHOD(Upper)
JINJA2CPP_STR_METHOD(Lower)
JINJA2CPP_STR_METHOD(Casefold)
JINJA2CPP_STR_METHOD(Swapcase)
JINJA2CPP_STR_METHOD(Title)
JINJA2CPP_STR_METHOD(Capitalize)
JINJA2CPP_STR_METHOD(Strip)
JINJA2CPP_STR_METHOD(Lstrip)
JINJA2CPP_STR_METHOD(Rstrip)
JINJA2CPP_STR_METHOD(Split)
JINJA2CPP_STR_METHOD(Rsplit)
JINJA2CPP_STR_METHOD(Splitlines)
JINJA2CPP_STR_METHOD(JoinMethod)
JINJA2CPP_STR_METHOD(Replace)
JINJA2CPP_STR_METHOD(Startswith)
JINJA2CPP_STR_METHOD(Endswith)
JINJA2CPP_STR_METHOD(Find)
JINJA2CPP_STR_METHOD(Rfind)
JINJA2CPP_STR_METHOD(Index)
JINJA2CPP_STR_METHOD(Rindex)
JINJA2CPP_STR_METHOD(Count)
// str.format and, with formatMap, str.format_map; Markup's escape each field (escape). A sandbox
// policy may format the string itself (ISandboxPolicy::Format)
InternalValue FormatString(const InternalValue& self, const CallParams& params, RenderContext& context, bool formatMap, bool escape)
{
    if (const auto* policy = context.GetSandboxPolicy())
    {
        if (auto formatted = policy->Format(self, formatMap, params, context))
        {
            return std::move(*formatted);
        }
    }
    return ApplyStringConverter(self, [&params, &context, formatMap, escape](auto sv) -> InternalValue {
        using CharT = typename decltype(sv)::value_type;
        return formatMap ? StrOps<CharT>::FormatMap(sv, params, context, escape) : StrOps<CharT>::Format(sv, params, context, escape);
    });
}
InternalValue StrFormat(const InternalValue& self, const CallParams& params, RenderContext& context)
{
    return FormatString(self, params, context, false, false);
}
InternalValue StrFormatMap(const InternalValue& self, const CallParams& params, RenderContext& context)
{
    return FormatString(self, params, context, true, false);
}
JINJA2CPP_STR_METHOD(Isdigit)
JINJA2CPP_STR_METHOD(Isdecimal)
JINJA2CPP_STR_METHOD(Isnumeric)
JINJA2CPP_STR_METHOD(Isascii)
JINJA2CPP_STR_METHOD(Isidentifier)
JINJA2CPP_STR_METHOD(Isprintable)
JINJA2CPP_STR_METHOD(Isalpha)
JINJA2CPP_STR_METHOD(Isalnum)
JINJA2CPP_STR_METHOD(Isspace)
JINJA2CPP_STR_METHOD(Isupper)
JINJA2CPP_STR_METHOD(Islower)
JINJA2CPP_STR_METHOD(Istitle)
JINJA2CPP_STR_METHOD(Zfill)
JINJA2CPP_STR_METHOD(Center)
JINJA2CPP_STR_METHOD(Ljust)
JINJA2CPP_STR_METHOD(Rjust)
JINJA2CPP_STR_METHOD(Partition)
JINJA2CPP_STR_METHOD(Rpartition)
JINJA2CPP_STR_METHOD(Removeprefix)
JINJA2CPP_STR_METHOD(Removesuffix)
JINJA2CPP_STR_METHOD(Expandtabs)
JINJA2CPP_STR_METHOD(Translate)
JINJA2CPP_STR_METHOD(Maketrans)

#undef JINJA2CPP_STR_METHOD

const MethodInfo StrMethods[] = {
    { "upper", StrUpper, false },
    { "lower", StrLower, false },
    { "casefold", StrCasefold, false },
    { "swapcase", StrSwapcase, false },
    { "title", StrTitle, false },
    { "capitalize", StrCapitalize, false },
    { "strip", StrStrip, false },
    { "lstrip", StrLstrip, false },
    { "rstrip", StrRstrip, false },
    { "split", StrSplit, false },
    { "rsplit", StrRsplit, false },
    { "splitlines", StrSplitlines, false },
    { "join", StrJoinMethod, false },
    { "replace", StrReplace, false },
    { "startswith", StrStartswith, false },
    { "endswith", StrEndswith, false },
    { "find", StrFind, false },
    { "rfind", StrRfind, false },
    { "index", StrIndex, false },
    { "rindex", StrRindex, false },
    { "count", StrCount, false },
    { "format", StrFormat, false },
    { "isdigit", StrIsdigit, false },
    { "isdecimal", StrIsdecimal, false },
    { "isnumeric", StrIsnumeric, false },
    { "isascii", StrIsascii, false },
    { "isidentifier", StrIsidentifier, false },
    { "isprintable", StrIsprintable, false },
    { "isalpha", StrIsalpha, false },
    { "isalnum", StrIsalnum, false },
    { "isspace", StrIsspace, false },
    { "isupper", StrIsupper, false },
    { "islower", StrIslower, false },
    { "istitle", StrIstitle, false },
    { "zfill", StrZfill, false },
    { "center", StrCenter, false },
    { "ljust", StrLjust, false },
    { "rjust", StrRjust, false },
    { "partition", StrPartition, false },
    { "rpartition", StrRpartition, false },
    { "removeprefix", StrRemoveprefix, false },
    { "removesuffix", StrRemovesuffix, false },
    { "format_map", StrFormatMap, false },
    { "expandtabs", StrExpandtabs, false },
    { "translate", StrTranslate, false },
    { "maketrans", StrMaketrans, false },
};

// ---------------------------------------------------------------------------------------
// Markup (markupsafe.Markup): the str methods return Markup, a list or tuple of strings one of
// Markup, and the text a method inserts (replace's new, a fill character, join's items) is
// escaped first

// Marks the strings a str method returned as Markup
InternalValue MarkupResult(InternalValue result)
{
    if (IsStringValue(result))
    {
        result.SetMarkup();
        return result;
    }
    auto list = AsList(result);
    if (!list || result.Kind() != ValueKind::List)
    {
        return result;
    }
    InternalValueList items;
    items.reserve(list->GetSize().value_or(0));
    for (auto item : *list)
    {
        item.SetMarkup(IsStringValue(item));
        items.push_back(std::move(item));
    }
    return list->IsTuple() ? MakeTuple(std::move(items)) : InternalValue(ListAdapter::CreateAdapter(std::move(items)));
}

constexpr size_t NoEscapedArg = static_cast<size_t>(-1);

// The str method fn on Markup; the positional argument escapedArg, if given, is escaped first
template<MethodFn fn, size_t escapedArg = NoEscapedArg>
InternalValue MarkupMethod(const InternalValue& self, const CallParams& params, RenderContext& context)
{
    if constexpr (escapedArg != NoEscapedArg)
    {
        if (params.posParams.size() > escapedArg)
        {
            CallParams escaped = params;
            escaped.posParams[escapedArg] = MarkupEscape(params.posParams[escapedArg], context.GetRendererCallback());
            return MarkupResult(fn(self, escaped, context));
        }
    }
    return MarkupResult(fn(self, params, context));
}

// Markup.join escapes every item, so items of any type join
InternalValue MarkupJoin(const InternalValue& self, const CallParams& params, RenderContext& context)
{
    CheckArgs(params, "join", 1, 1);
    bool isConverted = false;
    auto list = ConvertToList(params.posParams[0], isConverted, false);
    if (!isConverted)
    {
        Raise("can only join an iterable");
    }
    InternalValueList items;
    for (const auto& item : list)
    {
        items.push_back(MarkupEscape(item, context.GetRendererCallback()));
    }
    CallParams escaped;
    escaped.posParams.emplace_back(ListAdapter::CreateAdapter(std::move(items)));
    return MarkupResult(StrJoinMethod(self, escaped, context));
}

// markupsafe's EscapeFormatter: a policy's result is taken as it is, the engine's is Markup
InternalValue MarkupFormatString(const InternalValue& self, const CallParams& params, RenderContext& context, bool formatMap)
{
    if (const auto* policy = context.GetSandboxPolicy())
    {
        if (auto formatted = policy->Format(self, formatMap, params, context))
        {
            return std::move(*formatted);
        }
    }
    return MarkupResult(ApplyStringConverter(self, [&params, &context, formatMap](auto sv) -> InternalValue {
        using CharT = typename decltype(sv)::value_type;
        return formatMap ? StrOps<CharT>::FormatMap(sv, params, context, true) : StrOps<CharT>::Format(sv, params, context, true);
    }));
}
InternalValue MarkupFormat(const InternalValue& self, const CallParams& params, RenderContext& context)
{
    return MarkupFormatString(self, params, context, false);
}
InternalValue MarkupFormatMap(const InternalValue& self, const CallParams& params, RenderContext& context)
{
    return MarkupFormatString(self, params, context, true);
}

const MethodInfo MarkupMethods[] = {
    { "upper", MarkupMethod<StrUpper>, false },
    { "lower", MarkupMethod<StrLower>, false },
    { "casefold", MarkupMethod<StrCasefold>, false },
    { "swapcase", MarkupMethod<StrSwapcase>, false },
    { "title", MarkupMethod<StrTitle>, false },
    { "capitalize", MarkupMethod<StrCapitalize>, false },
    { "strip", MarkupMethod<StrStrip>, false },
    { "lstrip", MarkupMethod<StrLstrip>, false },
    { "rstrip", MarkupMethod<StrRstrip>, false },
    { "split", MarkupMethod<StrSplit>, false },
    { "rsplit", MarkupMethod<StrRsplit>, false },
    { "splitlines", MarkupMethod<StrSplitlines>, false },
    { "join", MarkupJoin, false },
    { "replace", MarkupMethod<StrReplace, 1>, false },
    { "startswith", StrStartswith, false },
    { "endswith", StrEndswith, false },
    { "find", StrFind, false },
    { "rfind", StrRfind, false },
    { "index", StrIndex, false },
    { "rindex", StrRindex, false },
    { "count", StrCount, false },
    { "format", MarkupFormat, false },
    { "isdigit", StrIsdigit, false },
    { "isdecimal", StrIsdecimal, false },
    { "isnumeric", StrIsnumeric, false },
    { "isascii", StrIsascii, false },
    { "isidentifier", StrIsidentifier, false },
    { "isprintable", StrIsprintable, false },
    { "isalpha", StrIsalpha, false },
    { "isalnum", StrIsalnum, false },
    { "isspace", StrIsspace, false },
    { "isupper", StrIsupper, false },
    { "islower", StrIslower, false },
    { "istitle", StrIstitle, false },
    { "zfill", MarkupMethod<StrZfill>, false },
    { "center", MarkupMethod<StrCenter, 1>, false },
    { "ljust", MarkupMethod<StrLjust, 1>, false },
    { "rjust", MarkupMethod<StrRjust, 1>, false },
    { "partition", MarkupMethod<StrPartition>, false },
    { "rpartition", MarkupMethod<StrRpartition>, false },
    { "removeprefix", MarkupMethod<StrRemoveprefix>, false },
    { "removesuffix", MarkupMethod<StrRemovesuffix>, false },
    { "format_map", MarkupFormatMap, false },
    { "expandtabs", MarkupMethod<StrExpandtabs>, false },
    // markupsafe wraps translate without escaping the table's strings; maketrans is str's
    { "translate", MarkupMethod<StrTranslate>, false },
    { "maketrans", StrMaketrans, false },
};
static_assert(std::size(MarkupMethods) == std::size(StrMethods), "Markup has every str method");

// ---------------------------------------------------------------------------------------
// list and tuple

ListRef ListOf(const InternalValue& self)
{
    auto list = AsList(self);
    if (!list)
    {
        Raise("not a list");
    }
    return *list;
}

InternalValueList& MutableItems(const InternalValue& self)
{
    auto* items = ListOf(self).GetMutableItems();
    // The call path makes the receiver mutable first (MakeMutable); a bound method kept its
    // receiver as it was read (`set f = l.append`), and a list of the host's data takes its copy
    // only now (docs/tasks/0049)
    if (!items)
    {
        if (IsMutable(MakeMutable(self)))
        {
            items = ListOf(self).GetMutableItems();
        }
    }
    if (!items)
    {
        Raise("this list cannot be changed");
    }
    return *items;
}

// The cell of the module's data `self` reads, if it has one
const BorrowedCell* ListCell(const InternalValue& self) noexcept
{
    try
    {
        const auto list = AsList(self);
        const auto* accessor = list ? list->GetAccessor() : nullptr;
        const auto* link = accessor ? accessor->GetCellLink() : nullptr;
        return link ? link->Get(*accessor) : nullptr;
    }
    catch (...)
    {
        // A link that cannot allocate finds no cell: the restore then copies as before
        return nullptr;
    }
}

InternalValue ListIndex(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "index", 1, 3);
    auto items = ListOf(self).ToValueList();
    auto start = SliceIndex(Arg(params, 1), items.size(), 0, "index");
    auto end = SliceIndex(Arg(params, 2), items.size(), items.size(), "index");
    for (auto n = start; n < end; ++n)
    {
        if (Equals(items[n], params.posParams[0]))
        {
            return static_cast<int64_t>(n);
        }
    }
    Raise(ListOf(self).IsTuple() ? "tuple.index(x): x not in tuple" : "list.index(x): x not in list");
}

InternalValue ListCount(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "count", 1, 1);
    int64_t result = 0;
    for (const auto& item : ListOf(self))
    {
        if (Equals(item, params.posParams[0]))
        {
            ++result;
        }
    }
    return result;
}

InternalValue ListAppend(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "append", 1, 1);
    auto& items = MutableItems(self);
    CheckSequenceSize(uint64_t{ items.size() } + 1, "a list");
    ReserveCounted(items, items.size() + 1);
    NoteContainerStore(self, params.posParams[0]);
    items.push_back(params.posParams[0]);
    return InternalValue(NoneValue());
}

InternalValue ListExtend(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "extend", 1, 1);
    bool isConverted = false;
    auto other = ConvertToList(params.posParams[0], isConverted);
    if (!isConverted)
    {
        Raise("'" + TypeName(params.posParams[0]) + "' object is not iterable");
    }
    // A snapshot first: l.extend(l) doubles the list, as in Python
    auto newItems = other.ToValueList();
    auto& items = MutableItems(self);
    CheckSequenceSize(uint64_t{ items.size() } + newItems.size(), "a list");
    ReserveCounted(items, items.size() + newItems.size());
    for (auto& item : newItems)
    {
        NoteContainerStore(self, item);
    }
    items.insert(items.end(), newItems.begin(), newItems.end());
    return InternalValue(NoneValue());
}

InternalValue ListInsert(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "insert", 2, 2);
    auto& items = MutableItems(self);
    auto idx = SliceIndex(params.posParams.data(), items.size(), 0, "insert");
    CheckSequenceSize(uint64_t{ items.size() } + 1, "a list");
    ReserveCounted(items, items.size() + 1);
    NoteContainerStore(self, params.posParams[1]);
    items.insert(items.begin() + static_cast<std::ptrdiff_t>(idx), params.posParams[1]);
    return InternalValue(NoneValue());
}

InternalValue ListPop(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "pop", 0, 1);
    auto& items = MutableItems(self);
    if (items.empty())
    {
        Raise("pop from empty list");
    }
    int64_t idx = params.posParams.empty() ? -1 : IntArg(params.posParams[0], "pop");
    if (idx < 0)
    {
        idx += static_cast<int64_t>(items.size());
    }
    if (idx < 0 || idx >= static_cast<int64_t>(items.size()))
    {
        Raise("pop index out of range");
    }
    CycleCollector::NoteDropped(items[static_cast<size_t>(idx)]);
    auto result = std::move(items[static_cast<size_t>(idx)]);
    items.erase(items.begin() + idx);
    return result;
}

InternalValue ListRemove(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "remove", 1, 1);
    auto& items = MutableItems(self);
    auto p = std::find_if(items.begin(), items.end(), [&params](const InternalValue& item) { return Equals(item, params.posParams[0]); });
    if (p == items.end())
    {
        Raise("list.remove(x): x not in list");
    }
    CycleCollector::NoteDropped(*p);
    items.erase(p);
    return InternalValue(NoneValue());
}

InternalValue ListReverse(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "reverse", 0, 0);
    auto& items = MutableItems(self);
    std::reverse(items.begin(), items.end());
    return InternalValue(NoneValue());
}

// list.sort(*, key=None, reverse=False), stable like Python's, comparing with `<` only. As in
// CPython the list looks empty while the keys are computed and compared, and a list changed
// meanwhile is an error; a key or comparison that fails leaves the list as it was.
InternalValue ListSort(const InternalValue& self, const CallParams& params, RenderContext& context)
{
    if (!params.posParams.empty())
    {
        Raise("sort() takes no positional arguments");
    }
    for (const auto& kw : params.kwParams)
    {
        if (kw.first != "key" && kw.first != "reverse")
        {
            Raise("'" + kw.first + "' is an invalid keyword argument for sort()");
        }
    }
    const auto* keyFn = ArgOrNone(params, 0, "key");
    const auto* reverseArg = Arg(params, 0, "reverse");
    bool reverse = false;
    if (reverseArg)
    {
        reverse = ConvertToBool(*reverseArg);
    }

    InternalValueList values;
    // A key call's module call can hand the list to the host, and the restore below then needs a
    // copy of the list to write into (D8). It is made here, before anything changes, so that the
    // restore does not allocate and a list whose key call failed is as it was (docs/tasks/0485)
    auto& taken = MutableItems(self);
    InternalValue spare;
    if (keyFn && CycleCollector::InModuleCall() && ListCell(self))
    {
        spare = InternalValue(ListAdapter::CreateAdapter(InternalValueList{}));
    }
    values.swap(taken);
    // The render's memory budget counted the buffer taken here (docs/tasks/0459)
    const std::uint64_t takenCapacity = values.capacity();
    // The items are found again after the key calls: a module call a key function makes can hand
    // the list's items to the host and give the list new ones (docs/tasks/0331, D8), and the
    // items the host was handed are never written again
    struct Restore
    {
        const InternalValue& self;
        InternalValueList& values;
        InternalValue& spare;
        Restore(const InternalValue& list, InternalValueList& taken, InternalValue& spareList)
            : self(list)
            , values(taken)
            , spare(spareList)
        {
        }
        Restore(const Restore&) = delete;
        Restore(Restore&&) = delete;
        Restore& operator=(const Restore&) = delete;
        Restore& operator=(Restore&&) = delete;
        ~Restore()
        {
            // Whatever a key call put into the list meanwhile is dropped, as in CPython
            if (values.empty())
            {
                return;
            }
            try
            {
                // The items the host was handed stay as they are; the list moves to the spare
                if (const auto* cell = AsList(spare) ? ListCell(self) : nullptr)
                {
                    cell->UnshareWith(spare);
                }
                MutableItems(self).swap(values);
            }
            catch (...) // NOLINT(bugprone-empty-catch): out of memory copying the list's items; it keeps what the key calls left
            {
            }
        }
    } restore{ self, values, spare };

    InternalValueList keys;
    if (keyFn)
    {
        keys.reserve(values.size());
        for (const auto& val : values)
        {
            CallParams keyParams;
            keyParams.posParams.push_back(val);
            keys.push_back(CallExpression::CallValue(context, *keyFn, keyParams));
        }
    }
    const auto& sortKeys = keyFn ? keys : values;
    std::vector<size_t> order(values.size());
    std::iota(order.begin(), order.end(), size_t{ 0 });
    // reverse=True keeps equal items in their order too
    StableSortValues(order.begin(), order.end(), [&sortKeys, reverse](size_t idx1, size_t idx2) {
        const auto& left = sortKeys[reverse ? idx2 : idx1];
        const auto& right = sortKeys[reverse ? idx1 : idx2];
        return ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, BinaryExpression::LogicalLt, BinaryExpression::CaseSensitive));
    });
    // Found before the items move: it may copy the list (D8), and when that fails the restore
    // guard still holds them
    auto& items = MutableItems(self);
    // CPython's allocated == -1 sentinel: the list was left with no storage, and any change
    // that put an item in it gave it some, even when a later pop emptied it again
    const bool modified = items.capacity() != 0;
    InternalValueList sorted;
    sorted.reserve(values.size());
    for (auto idx : order)
    {
        sorted.push_back(std::move(values[idx]));
    }
    values.clear();
    // The taken buffer, and any a key call grew meanwhile, are freed for one that fits the items: the budget gives
    // back the difference
    const std::uint64_t replacedCapacity = takenCapacity + items.capacity();
    items = std::move(sorted);
    ReleaseRenderMemory((replacedCapacity - items.capacity()) * sizeof(InternalValue));
    if (modified)
    {
        Raise("list modified during sort");
    }
    return InternalValue(NoneValue());
}

// The items a list lets go of (CycleCollector::NoteDropped)
void NoteDroppedItems(const InternalValueList& items)
{
    if (CycleCollector::InModuleCall())
    {
        for (const auto& item : items)
        {
            CycleCollector::NoteDropped(item);
        }
    }
}

InternalValue ListClear(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "clear", 0, 0);
    auto& items = MutableItems(self);
    NoteDroppedItems(items);
    items.clear();
    return InternalValue(NoneValue());
}

InternalValue ListCopy(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "copy", 0, 0);
    return ListAdapter::CreateAdapter(ListOf(self).ToValueList());
}

const MethodInfo ListMethods[] = {
    { "index", ListIndex, false },
    { "count", ListCount, false },
    { "append", ListAppend, true },
    { "extend", ListExtend, true },
    { "insert", ListInsert, true },
    { "pop", ListPop, true },
    { "remove", ListRemove, true },
    { "reverse", ListReverse, true },
    { "sort", ListSort, true },
    { "clear", ListClear, true },
    { "copy", ListCopy, false },
};

const MethodInfo TupleMethods[] = {
    { "index", ListIndex, false },
    { "count", ListCount, false },
};

// ---------------------------------------------------------------------------------------
// dict

MapRef MapOf(const InternalValue& self)
{
    auto map = AsMap(self);
    if (!map)
    {
        Raise("not a dict");
    }
    return *map;
}

TemplateDict& MutableDict(const InternalValue& self)
{
    auto* items = MapOf(self).GetMutableItems();
    // As MutableItems does for a list
    if (!items)
    {
        if (IsMutable(MakeMutable(self)))
        {
            items = MapOf(self).GetMutableItems();
        }
    }
    if (!items)
    {
        Raise("this dict cannot be changed");
    }
    return *items;
}

InternalValue DictKeys(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "keys", 0, 0);
    return ListAdapter::CreateAdapter(MapOf(self).GetKeyValues());
}

InternalValue DictValues(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "values", 0, 0);
    const auto& map = MapOf(self);
    InternalValueList result;
    for (auto& entry : map.GetEntries())
    {
        result.push_back(std::move(entry.value));
    }
    InternalValue list = ListAdapter::CreateAdapter(std::move(result));
    if (self.ShouldExtendLifetime())
    {
        list.SetParentData(self);
    }
    return list;
}

InternalValue DictItems(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "items", 0, 0);
    const auto& map = MapOf(self);
    InternalValueList result;
    for (auto& entry : map.GetEntries())
    {
        result.push_back(MakeTuple({ std::move(entry.key), std::move(entry.value) }));
    }
    InternalValue list = ListAdapter::CreateAdapter(std::move(result));
    if (self.ShouldExtendLifetime())
    {
        list.SetParentData(self);
    }
    return list;
}

InternalValue DictGet(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "get", 1, 2);
    const auto& map = MapOf(self);
    const auto& key = params.posParams[0];
    if (IsStringValue(key))
    {
        const auto name = AsString(key);
        if (map.HasValue(name))
        {
            return map.GetValueByName(name);
        }
    }
    else
    {
        // Python hashes the key first, also when the dict has no such key
        CheckHashable(key);
        if (auto found = map.FindItem(key))
        {
            return std::move(*found);
        }
    }
    return params.posParams.size() > 1 ? params.posParams[1] : InternalValue(NoneValue());
}

InternalValue DictSetdefault(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "setdefault", 1, 2);
    auto& items = MutableDict(self);
    const auto& key = params.posParams[0];
    CheckHashable(key);
    auto p = items.Find(key);
    if (p != items.end())
    {
        return p->second;
    }
    InternalValue value = params.posParams.size() > 1 ? params.posParams[1] : InternalValue(NoneValue());
    // A key that is not a string is kept too, and a tuple key can hold a container
    NoteContainerStore(self, key);
    NoteContainerStore(self, value);
    items.Set(key, value);
    return value;
}

using DictUpdates = std::vector<std::pair<InternalValue, InternalValue>>;

// The key-value pairs of an iterable of pairs, d.update([(k, v), ...])
void CollectPairUpdates(const InternalValue& other, DictUpdates& updates)
{
    bool isConverted = false;
    auto pairs = ConvertToList(other, isConverted);
    if (!isConverted || IsStringValue(other))
    {
        Raise("'" + TypeName(other) + "' object is not iterable");
    }
    for (const auto& pairVal : pairs)
    {
        bool isPair = false;
        auto pairList = ConvertToList(pairVal, isPair);
        auto pair = isPair ? pairList.ToValueList() : InternalValueList();
        if (!isPair || pair.size() != 2)
        {
            Raise("dictionary update sequence element has wrong length; 2 is required");
        }
        // A failing update leaves the dict as it was: the keys are checked before any is stored
        CheckHashable(pair[0]);
        updates.emplace_back(pair[0], pair[1]);
    }
}

// The key-value pairs of the positional argument of d.update, a mapping or an iterable of pairs
void CollectPositionalUpdates(const InternalValue& other, DictUpdates& updates)
{
    // An object such as namespace() is no mapping, and not iterable either
    if (auto otherMap = AsMap(other); otherMap && !otherMap->GetObjectClass())
    {
        for (auto& entry : otherMap->GetEntries())
        {
            updates.emplace_back(std::move(entry.key), std::move(entry.value));
        }
    }
    else
    {
        CollectPairUpdates(other, updates);
    }
}

InternalValue DictUpdate(const InternalValue& self, const CallParams& params, RenderContext&)
{
    if (params.posParams.size() > 1)
    {
        Raise(fmt::format("update expected at most 1 argument, got {}", params.posParams.size()));
    }
    // Collect first: d.update(d) and failing pairs leave d as it was
    DictUpdates updates;
    if (!params.posParams.empty())
    {
        CollectPositionalUpdates(params.posParams[0], updates);
    }
    for (const auto& [name, value] : params.kwParams)
    {
        updates.emplace_back(InternalValue(name), value);
    }
    auto& items = MutableDict(self);
    for (auto& [key, value] : updates)
    {
        NoteContainerStore(self, key);
        NoteContainerStore(self, value);
        if (CycleCollector::InModuleCall())
        {
            if (auto old = items.Find(key); old != items.end())
            {
                CycleCollector::NoteDropped(old->second);
            }
        }
    }
    for (auto& [key, value] : updates)
    {
        items.Set(key, std::move(value));
    }
    return InternalValue(NoneValue());
}

InternalValue DictPop(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "pop", 1, 2);
    auto& items = MutableDict(self);
    const auto& key = params.posParams[0];
    CheckHashable(key);
    auto p = items.Find(key);
    if (p == items.end())
    {
        if (params.posParams.size() > 1)
        {
            return params.posParams[1];
        }
        std::string spelling;
        Apply<visitors::ValueRenderer<char>>(key, spelling, true);
        Raise("KeyError: " + spelling);
    }
    CycleCollector::NoteDropped(p->second);
    auto result = p->second;
    items.Erase(p);
    return result;
}

InternalValue DictPopitem(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "popitem", 0, 0);
    auto& items = MutableDict(self);
    if (items.empty())
    {
        Raise("popitem(): dictionary is empty");
    }
    auto last = std::prev(items.end());
    CycleCollector::NoteDropped(last->second);
    auto result = MakeTuple({ items.KeyOf(last->first), last->second });
    items.Erase(last);
    return result;
}

InternalValue DictCopy(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "copy", 0, 0);
    const auto& map = MapOf(self);
    if (const auto* items = map.GetMutableItems())
    {
        return CreateMapAdapter(TemplateDict(*items));
    }
    TemplateDict result;
    for (auto& entry : map.GetEntries())
    {
        result.Set(entry.key, std::move(entry.value));
    }
    return CreateMapAdapter(std::move(result));
}

InternalValue DictClear(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "clear", 0, 0);
    auto& items = MutableDict(self);
    if (CycleCollector::InModuleCall())
    {
        for (const auto& [key, value] : items)
        {
            CycleCollector::NoteDropped(value);
        }
    }
    items.Clear();
    return InternalValue(NoneValue());
}

const MethodInfo DictMethods[] = {
    { "keys", DictKeys, false },
    { "values", DictValues, false },
    { "items", DictItems, false },
    { "get", DictGet, false },
    { "setdefault", DictSetdefault, true },
    { "update", DictUpdate, true },
    { "pop", DictPop, true },
    { "popitem", DictPopitem, true },
    { "copy", DictCopy, false },
    { "clear", DictClear, true },
};

// ---------------------------------------------------------------------------------------
// int, bool and float

InternalValue IntBitLength(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "bit_length", 0, 0);
    if (const auto big = AsBigInt(self))
    {
        return static_cast<int64_t>(BitLength(*big));
    }
    auto value = IntArg(self, "bit_length");
    uint64_t magnitude = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    int64_t bits = 0;
    for (; magnitude != 0; magnitude >>= 1)
    {
        ++bits;
    }
    return bits;
}

InternalValue FloatIsInteger(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "is_integer", 0, 0);
    auto value = *GetIf<double>(&self);
    return InternalValue(std::isfinite(value) && std::floor(value) == value);
}

// An int, or a bool as the int it is (True.conjugate() is 1)
InternalValue AsIntValue(const InternalValue& self)
{
    if (const auto* b = GetIf<bool>(&self))
    {
        return static_cast<int64_t>(*b ? 1 : 0);
    }
    return self;
}

InternalValue IntBitCount(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "bit_count", 0, 0);
    if (const auto big = AsBigInt(self))
    {
        const auto bits = MagnitudeInBase(*big, 2);
        return static_cast<int64_t>(std::count(bits.begin(), bits.end(), '1'));
    }
    auto value = IntArg(self, "bit_count");
    uint64_t magnitude = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    int64_t bits = 0;
    for (; magnitude != 0; magnitude &= magnitude - 1)
    {
        ++bits;
    }
    return bits;
}

InternalValue IntConjugate(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "conjugate", 0, 0);
    return AsIntValue(self);
}

InternalValue IntAsIntegerRatio(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "as_integer_ratio", 0, 0);
    return MakeTuple({ AsIntValue(self), InternalValue(int64_t{ 1 }) });
}

// int.is_integer (Python 3.12): always true
InternalValue IntIsInteger(const InternalValue&, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "is_integer", 0, 0);
    return InternalValue(true);
}

InternalValue FloatConjugate(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "conjugate", 0, 0);
    return self;
}

// 2**exponent as an int, a big one past int64
InternalValue PowerOfTwo(int exponent)
{
    if (exponent < 63)
    {
        return static_cast<int64_t>(uint64_t{ 1 } << exponent);
    }
    return IntegerFromDigits("1" + std::string(static_cast<size_t>(exponent), '0'), 2, false);
}

// CPython's float_as_integer_ratio: the fraction in lowest terms, whose denominator is a power of two
InternalValue FloatAsIntegerRatio(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "as_integer_ratio", 0, 0);
    const auto value = *GetIf<double>(&self);
    if (std::isinf(value))
    {
        Raise("OverflowError: cannot convert Infinity to integer ratio"s);
    }
    if (std::isnan(value))
    {
        Raise("ValueError: cannot convert NaN to integer ratio"s);
    }
    if (std::floor(value) == value)
    {
        return MakeTuple({ IntegerFromDouble(value), InternalValue(int64_t{ 1 }) });
    }
    int exponent = 0;
    const double fraction = std::frexp(std::fabs(value), &exponent);
    // fraction * 2**53 is the 53-bit significand; value = significand * 2**(exponent - 53)
    auto significand = static_cast<uint64_t>(std::ldexp(fraction, std::numeric_limits<double>::digits));
    exponent -= std::numeric_limits<double>::digits;
    // Not an integer, so the exponent stays negative while the trailing zeros go
    while ((significand & 1) == 0)
    {
        significand >>= 1;
        ++exponent;
    }
    const auto numerator = static_cast<int64_t>(significand);
    return MakeTuple({ InternalValue(value < 0 ? -numerator : numerator), PowerOfTwo(-exponent) });
}

// CPython's float_hex: [-]0x<digit>.<13 hex digits>p<exponent>, subnormals as 0x0.<digits>p-1022
InternalValue FloatHex(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "hex", 0, 0);
    const auto value = *GetIf<double>(&self);
    if (std::isnan(value))
    {
        return InternalValue("nan"s);
    }
    if (std::isinf(value))
    {
        return InternalValue(value < 0 ? "-inf"s : "inf"s);
    }
    const std::string sign = std::signbit(value) ? "-" : "";
    if (value == 0)
    {
        return InternalValue(sign + "0x0.0p+0");
    }
    int exponent = 0;
    double fraction = std::frexp(std::fabs(value), &exponent);
    const int shift = 1 - std::max(std::numeric_limits<double>::min_exponent - exponent, 0);
    fraction = std::ldexp(fraction, shift);
    exponent -= shift;
    constexpr char digits[] = "0123456789abcdef";
    std::string result = sign + "0x";
    auto digit = static_cast<int>(fraction);
    result.push_back(digits[digit]);
    fraction -= digit;
    result.push_back('.');
    constexpr int fractionDigits = (std::numeric_limits<double>::digits - 1) / 4;
    for (int n = 0; n < fractionDigits; ++n)
    {
        fraction *= 16.0;
        digit = static_cast<int>(fraction);
        result.push_back(digits[digit]);
        fraction -= digit;
    }
    return InternalValue(result + fmt::format("p{:+d}", exponent));
}

// The methods of int, bool and float the engine has; bytes ones (to_bytes, from_bytes) and the
// alternate constructor float.fromhex are not (docs/python.md, docs/tasks/0403)
const MethodInfo IntMethods[] = {
    { "bit_length", IntBitLength, false },
    { "bit_count", IntBitCount, false },
    { "conjugate", IntConjugate, false },
    { "as_integer_ratio", IntAsIntegerRatio, false },
    { "is_integer", IntIsInteger, false },
};

const MethodInfo FloatMethods[] = {
    { "is_integer", FloatIsInteger, false },
    { "conjugate", FloatConjugate, false },
    { "as_integer_ratio", FloatAsIntegerRatio, false },
    { "hex", FloatHex, false },
};

// The data attributes of int, bool and float (docs/tasks/0374): x.real, x.imag, and for an int
// x.numerator and x.denominator. A bool is an int here, as in Python (True.real is 1)
constexpr std::string_view NumberDataAttributes[] = { "real", "imag", "numerator", "denominator" };

template<size_t N>
const MethodInfo* FindIn(const MethodInfo (&table)[N], std::string_view name)
{
    for (auto& m : table)
    {
        if (name == m.name)
        {
            return &m;
        }
    }
    return nullptr;
}

const MethodInfo* FindMethodByKind(const InternalValue& self, std::string_view name)
{
    switch (self.Kind())
    {
    case ValueKind::String:
        return self.IsMarkup() ? FindIn(MarkupMethods, name) : FindIn(StrMethods, name);
    case ValueKind::List:
        if (auto list = AsList(self))
        {
            // An iterator (a lazy filter's generator) has none of a list's methods (docs/tasks/0383)
            if (list->GetIteratorKind() != IteratorKind::None)
            {
                return nullptr;
            }
            return list->IsTuple() || list->GetRangeInfo() ? FindIn(TupleMethods, name) : FindIn(ListMethods, name);
        }
        return nullptr;
    case ValueKind::Map:
        if (auto map = AsMap(self))
        {
            // An object (KeysOnly) has no dict methods, and one of the host language only its own
            const auto policy = map->GetAttrPolicy();
            return policy == MapAttrPolicy::KeysOnly || policy == MapAttrPolicy::Host ? nullptr : FindIn(DictMethods, name);
        }
        return nullptr;
    case ValueKind::Int:
    case ValueKind::BigInt:
    case ValueKind::Bool:
        return FindIn(IntMethods, name);
    case ValueKind::Double:
        return FindIn(FloatMethods, name);
    default:
        return nullptr;
    }
}
} // namespace

bool IsMethodName(std::string_view name)
{
    // The parser asks for every attribute: the names of each length are a short sorted list, so
    // most attributes (a, b, name, ...) are ruled out by their length or a compare or two
    static constexpr std::size_t maxLength = 32;
    static const auto names = [] {
        std::array<std::vector<std::string_view>, maxLength + 1> result;
        auto add = [&result](const auto& table) {
            for (const auto& method : table)
            {
                result[std::min(method.name.size(), maxLength)].push_back(method.name);
            }
        };
        add(StrMethods);
        add(ListMethods);
        add(DictMethods);
        add(IntMethods);
        add(FloatMethods);
        // The data attributes take the same getattr path
        for (const auto attr : NumberDataAttributes)
        {
            result[std::min(attr.size(), maxLength)].push_back(attr);
        }
        constexpr std::string_view classAttribute = "__class__";
        result[classAttribute.size()].push_back(classAttribute);
        for (auto& sameLength : result)
        {
            std::sort(sameLength.begin(), sameLength.end());
        }
        return result;
    }();
    const auto& sameLength = names[std::min(name.size(), maxLength)];
    return std::binary_search(sameLength.begin(), sameLength.end(), name);
}

namespace
{
std::optional<InternalValue> NumberDataAttr(const InternalValue& self, std::string_view name)
{
    const auto kind = self.Kind();
    if (kind == ValueKind::Double)
    {
        if (name == "real")
        {
            return self;
        }
        if (name == "imag")
        {
            return InternalValue(0.0);
        }
        return std::nullopt;
    }
    if (kind != ValueKind::Int && kind != ValueKind::BigInt && kind != ValueKind::Bool)
    {
        return std::nullopt;
    }
    if (name == "real" || name == "numerator")
    {
        if (const auto* flag = GetIf<bool>(&self))
        {
            return InternalValue(static_cast<int64_t>(*flag ? 1 : 0));
        }
        return self;
    }
    if (name == "imag")
    {
        return InternalValue(static_cast<int64_t>(0));
    }
    if (name == "denominator")
    {
        return InternalValue(static_cast<int64_t>(1));
    }
    return std::nullopt;
}
} // namespace

std::optional<InternalValue> GetNumberDataAttr(const InternalValue& self, std::string_view name, const RenderContext* context)
{
    auto attr = NumberDataAttr(self, name);
    if (attr && context && context->GetSandboxPolicy())
    {
        if (auto replacement = SandboxedNativeAttribute(context, self, std::string(name)))
        {
            return replacement;
        }
    }
    return attr;
}

std::optional<InternalValue> ClassAttribute(const InternalValue& obj, std::string_view name, const RenderContext* context)
{
    if (name != "__class__")
    {
        return std::nullopt;
    }
    // A host object answers for itself; a map's own `__class__` key is the attribute of a map whose keys come first
    if (GetHostObject(obj))
    {
        return std::nullopt;
    }
    // The sandbox refuses it as Jinja2's does, by its name
    if (context && context->GetSandboxPolicy())
    {
        if (auto replacement = SandboxedNativeAttribute(context, obj, std::string(name)))
        {
            return replacement;
        }
    }
    return MakeClassObject(obj);
}

bool IsDictMethodName(std::string_view name)
{
    return FindIn(DictMethods, name) != nullptr;
}

const MethodInfo* FindMethod(const InternalValue& self, std::string_view name)
{
    return FindMethodByKind(self, name);
}

InternalValue MakeBoundMethod(const InternalValue& self, const MethodInfo& method)
{
    Callable result(Callable::GlobalFunc, BoundMethodCall{ self, method.invoke });
    // The name its repr prints (<built-in method upper of str object at 0x...>, docs/tasks/0368);
    // without the object, which a string has no identity for, so that it compares as before.
    // The method table's names are literals
    result.SetBoundMethod(nullptr, method.name.data()); // NOLINT(bugprone-suspicious-stringview-data-usage)
    result.SetAttributes(std::make_shared<const InternalValueMap>(InternalValueMap{ { "__self__", self } }));
    return result;
}

bool IsReadOnlyContainers(const RenderContext& context)
{
    return detail::IsReadOnlyDialect(context.GetDialect());
}

InternalValue UnsafeMethod(const InternalValue& obj, std::string_view name, const RenderContext& context)
{
    // ImmutableSandboxedEnvironment.getattr: unsafe_undefined, a SecurityError when used
    return MakeUnsafeUndefined(context, "access to attribute '" + std::string(name) + "' of '" + TypeName(obj) + "' object is unsafe.");
}

namespace
{
// FindMethod, but a method that changes its receiver is unsafe (`unsafe` set, null returned)
// where containers are read-only
const MethodInfo* FindSafeMethod(const InternalValue& obj, std::string_view name, const RenderContext* context, bool& unsafe)
{
    const auto* method = FindMethod(obj, name);
    unsafe = method != nullptr && method->isMutating && context != nullptr && IsReadOnlyContainers(*context);
    return unsafe ? nullptr : method;
}
} // namespace

std::optional<InternalValue> SandboxedSyntheticAttribute(RenderContext* context, const InternalValue& obj, const std::string& name)
{
    if (!context || !context->GetSandboxPolicy())
    {
        return std::nullopt;
    }
    const auto map = AsMap(obj);
    if ((map && (map->GetObjectClass() || map->IsNamespace())) || GetIf<Callable>(&obj))
    {
        return SandboxedNativeAttribute(context, obj, name);
    }
    return std::nullopt;
}

InternalValue GetAttr(const InternalValue& obj, const std::string& name, RenderContext* context)
{
    bool unsafe = false;
    const auto* method = FindSafeMethod(obj, name, context, unsafe);
    if (method || unsafe)
    {
        auto map = AsMap(obj);
        if (!map || map->GetAttrPolicy() == MapAttrPolicy::MethodsFirst || !map->HasValue(name))
        {
            if (unsafe)
            {
                return UnsafeMethod(obj, name, *context);
            }
            // A sandbox policy decides about the method too (IEngineSandboxPolicy::NativeAttribute)
            if (context && context->GetSandboxPolicy())
            {
                if (auto replacement = SandboxedNativeAttribute(context, obj, name))
                {
                    return std::move(*replacement);
                }
            }
            return MakeBoundMethod(obj, *method);
        }
    }
    if (auto attr = GetNumberDataAttr(obj, name, context))
    {
        return std::move(*attr);
    }
    if (auto cls = ClassAttribute(obj, name, context))
    {
        return std::move(*cls);
    }
    return Subscript(obj, name, context);
}

InternalValue GetItem(const InternalValue& obj, const InternalValue& key, RenderContext* context)
{
    auto result = Subscript(obj, key, context);
    // A character of Markup is Markup
    if (obj.IsMarkup() && IsStringValue(result) && IsStringValue(obj))
    {
        result.SetMarkup();
        return result;
    }
    if (result.IsUndefined() && IsStringValue(key))
    {
        bool unsafe = false;
        if (const auto* method = FindSafeMethod(obj, AsString(key), context, unsafe))
        {
            if (context && context->GetSandboxPolicy())
            {
                if (auto replacement = SandboxedNativeAttribute(context, obj, AsString(key)))
                {
                    return std::move(*replacement);
                }
            }
            return MakeBoundMethod(obj, *method);
        }
        if (unsafe)
        {
            return UnsafeMethod(obj, AsString(key), *context);
        }
        // A number has no items: Jinja2's getitem falls back to getattr, its data attributes (0374)
        if (auto attr = GetNumberDataAttr(obj, AsString(key), context))
        {
            return std::move(*attr);
        }
        if (auto cls = ClassAttribute(obj, AsString(key), context))
        {
            return std::move(*cls);
        }
    }
    return result;
}

bool IsMutatingName(std::string_view name)
{
    const auto* listMethod = FindIn(ListMethods, name);
    const auto* dictMethod = FindIn(DictMethods, name);
    return (listMethod != nullptr && listMethod->isMutating) || (dictMethod != nullptr && dictMethod->isMutating);
}

void NoteContainerStore(const InternalValue& container, const InternalValue& item)
{
    const auto kind = item.Kind();
    // Any other value refers to objects only through the anchor of the container it was read
    // from (SetParentData): an undefined `loop.nope` keeps the loop's frame (docs/tasks/0444)
    const auto* object = detail::InternalValueAccess::Object(item);
    if (kind == ValueKind::List || kind == ValueKind::Map || kind == ValueKind::Callable || kind == ValueKind::KeyValuePair || (object && object->IsTraversable()))
    {
        CycleCollector::NoteStore(container);
    }
}

void StoreItem(const InternalValue& container, const InternalValue& key, InternalValue value)
{
    if (auto list = AsList(container))
    {
        auto* items = list->GetMutableItems();
        const auto* idxVal = GetIf<int64_t>(&key);
        if (!items || !idxVal)
        {
            return;
        }
        auto idx = *idxVal < 0 ? *idxVal + static_cast<int64_t>(items->size()) : *idxVal;
        if (idx >= 0 && idx < static_cast<int64_t>(items->size()))
        {
            (*items)[static_cast<size_t>(idx)] = std::move(value);
        }
    }
    else if (auto map = AsMap(container))
    {
        auto* items = map->GetMutableItems();
        if (items)
        {
            items->Set(key, std::move(value));
        }
    }
}

bool IsContainer(const InternalValue& value)
{
    return value.Kind() == ValueKind::List || value.Kind() == ValueKind::Map;
}

bool IsMutable(const InternalValue& value)
{
    if (auto list = AsList(value))
    {
        return list->GetMutableItems() != nullptr;
    }
    if (auto map = AsMap(value))
    {
        // A host object changes through its own methods (`x.append` is its attribute): the
        // engine never copies it to change the copy
        return map->IsHost() || map->GetMutableItems() != nullptr;
    }
    return false;
}

InternalValue MakeMutable(const InternalValue& value)
{
    if (IsMutable(value))
    {
        return value;
    }
    bool extendLifetime = value.ShouldExtendLifetime();
    if (auto list = AsList(value))
    {
        // Tuples and ranges are never changed in place: only their read-only methods exist
        if (list->IsTuple() || list->GetRangeInfo())
        {
            return value;
        }
        auto items = list->ToValueList();
        if (extendLifetime)
        {
            const auto* accessor = list->GetAccessor();
            const auto anchor = accessor ? accessor->LifetimeAnchor(value) : value;
            for (auto& item : items)
            {
                item.SetParentData(anchor);
            }
        }
        InternalValue copy(ListAdapter::CreateAdapter(std::move(items)));
        // The host's list becomes the copy for every name of it in the render (docs/tasks/0049)
        const auto* accessor = list->GetAccessor();
        return accessor && accessor->AdoptCopy(value, copy) ? value : copy;
    }
    if (auto map = AsMap(value))
    {
        TemplateDict items;
        const auto* mapAccessor = map->GetAccessor();
        const auto anchor = extendLifetime && mapAccessor ? mapAccessor->LifetimeAnchor(value) : value;
        for (auto& [key, item] : map->GetEntries())
        {
            if (extendLifetime)
            {
                item.SetParentData(anchor);
            }
            items.Set(key, std::move(item));
        }
        InternalValue copy(CreateMapAdapter(std::move(items)));
        const auto* accessor = map->GetAccessor();
        return accessor && accessor->AdoptCopy(value, copy) ? value : copy;
    }
    return value;
}

void ThrowNoAttribute(const InternalValue& obj, const std::string& name)
{
    Raise("'" + TypeName(obj) + " object' has no attribute '" + name + "'");
}

} // namespace methods
} // namespace v2
} // namespace jinja2
