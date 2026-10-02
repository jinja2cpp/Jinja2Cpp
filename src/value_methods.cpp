#include "value_methods.h"

#include "expression_evaluator.h"
#include "render_context.h"
#include "unicode_tables.h"
#include "value_visitors.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <unordered_set>

namespace jinja2
{
namespace methods
{
namespace
{
[[noreturn]] void Raise(const std::string& message)
{
    throw std::runtime_error(message);
}

bool IsStringValue(const InternalValue& val)
{
    const auto& data = val.GetData();
    return std::get_if<std::string>(&data) != nullptr || std::get_if<TargetString>(&data) != nullptr || std::get_if<TargetStringView>(&data) != nullptr;
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

// Mapping keys are strings in the value model (task 0036): other keys by their printed form
std::string KeyString(const InternalValue& key)
{
    if (IsStringValue(key))
        return AsString(key);
    std::string result;
    Apply<visitors::ValueRenderer<char>>(key, result);
    return result;
}

// Positional argument `idx`, or the keyword argument `kwName` when the method takes one
const InternalValue* Arg(const CallParams& params, size_t idx, const char* kwName = nullptr)
{
    if (idx < params.posParams.size())
        return &params.posParams[idx];
    if (kwName != nullptr)
    {
        auto p = params.kwParams.find(kwName);
        if (p != params.kwParams.end())
            return &p->second;
    }
    return nullptr;
}

const InternalValue* ArgOrNone(const CallParams& params, size_t idx, const char* kwName = nullptr)
{
    const auto* arg = Arg(params, idx, kwName);
    return arg == nullptr || arg->IsNone() ? nullptr : arg;
}

// Python's arity check; keyword arguments are accepted only where Python takes them
void CheckArgs(const CallParams& params, const char* name, size_t minArgs, size_t maxArgs, std::initializer_list<const char*> kwNames = {})
{
    for (const auto& kw : params.kwParams)
    {
        if (std::find_if(kwNames.begin(), kwNames.end(), [&kw](const char* n) { return kw.first == n; }) == kwNames.end())
            Raise(std::string(name) + "() got an unexpected keyword argument '" + kw.first + "'");
    }
    auto count = params.posParams.size() + params.kwParams.size();
    if (count < minArgs || params.posParams.size() > maxArgs)
    {
        if (minArgs == maxArgs)
            Raise(fmt::format("{}() takes exactly {} argument{} ({} given)", name, minArgs, minArgs == 1 ? "" : "s", count));
        if (count < minArgs)
            Raise(fmt::format("{}() takes at least {} argument{} ({} given)", name, minArgs, minArgs == 1 ? "" : "s", count));
        Raise(fmt::format("{}() takes at most {} argument{} ({} given)", name, maxArgs, maxArgs == 1 ? "" : "s", count));
    }
}

// The largest width or precision a method pads to: Python raises MemoryError or
// OverflowError well before a template could allocate this much
constexpr int64_t MaxWidth = int64_t(1) << 28;

int64_t WidthArg(int64_t width)
{
    if (width > MaxWidth)
        Raise("width or precision too big");
    return width;
}

int64_t IntArg(const InternalValue& val, const char* name)
{
    if (const auto* i = GetIf<int64_t>(&val))
        return *i;
    if (const auto* b = GetIf<bool>(&val))
        return *b ? 1 : 0;
    Raise(std::string(name) + "(): '" + TypeName(val) + "' object cannot be interpreted as an integer");
}

// Python's slice bounds: a negative index counts from the end, then it is clamped
size_t SliceIndex(const InternalValue* val, size_t len, size_t def, const char* name)
{
    if (val == nullptr || val->IsNone())
        return def;
    auto idx = IntArg(*val, name);
    if (idx < 0)
        idx += static_cast<int64_t>(len);
    return static_cast<size_t>(std::min<int64_t>(std::max<int64_t>(idx, 0), static_cast<int64_t>(len)));
}

InternalValue MakeTuple(InternalValueList items)
{
    return ListAdapter::CreateAdapter(std::move(items)).MarkAsTuple();
}

// ---------------------------------------------------------------------------------------
// str

inline uint32_t CodePointOf(std::string_view ch)
{
    auto lead = static_cast<unsigned char>(ch[0]);
    if (lead < 0x80 || ch.size() == 1)
        return lead;
    uint32_t value = lead & (lead >= 0xF0 ? 0x07 : lead >= 0xE0 ? 0x0F
                                                                : 0x1F);
    for (size_t n = 1; n < ch.size(); ++n)
        value = (value << 6) | (static_cast<unsigned char>(ch[n]) & 0x3F);
    return value;
}

inline uint32_t CodePointOf(std::wstring_view ch)
{
    auto unit = static_cast<uint32_t>(ch[0]);
    if (ch.size() == 2 && unit >= 0xD800 && unit <= 0xDBFF)
        return 0x10000 + ((unit - 0xD800) << 10) + (static_cast<uint32_t>(ch[1]) - 0xDC00);
    return unit;
}

// Case mapping covers ASCII, as the upper, lower and title filters do
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
            Raise(std::string(name) + "() argument must be str, not " + TypeName(val));
        return *str;
    }

    // The characters [b, e) of chars as a string
    static Str Join(const Chars& chars, size_t b, size_t e)
    {
        if (b >= e)
            return Str();
        return Str(chars[b].data(), chars[e - 1].data() + chars[e - 1].size());
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

    template<typename Fn>
    static Str MapChars(View self, Fn&& fn)
    {
        Str result;
        result.reserve(self.size());
        for (auto ch : SplitCodePoints(self))
        {
            auto cp = CodePointOf(ch);
            if (cp < 0x80)
                result.push_back(static_cast<CharT>(fn(cp)));
            else
                result.append(ch.begin(), ch.end());
        }
        return result;
    }

    static InternalValue Upper(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "upper", 0, 0);
        return Result(MapChars(self, [](uint32_t cp) { return IsAsciiLower(cp) ? cp - 32 : cp; }));
    }
    static InternalValue Lower(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "lower", 0, 0);
        return Result(MapChars(self, [](uint32_t cp) { return IsAsciiUpper(cp) ? cp + 32 : cp; }));
    }
    static InternalValue Swapcase(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "swapcase", 0, 0);
        return Result(MapChars(self, [](uint32_t cp) {
            if (IsAsciiUpper(cp))
                return cp + 32;
            return IsAsciiLower(cp) ? cp - 32 : cp;
        }));
    }
    static InternalValue Title(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "title", 0, 0);
        bool prevCased = false;
        Str result;
        for (auto ch : SplitCodePoints(self))
        {
            auto cp = CodePointOf(ch);
            if (cp < 0x80)
            {
                bool isCased = IsAsciiUpper(cp) || IsAsciiLower(cp);
                if (isCased)
                    cp = prevCased ? (IsAsciiUpper(cp) ? cp + 32 : cp) : (IsAsciiLower(cp) ? cp - 32 : cp);
                prevCased = isCased;
                result.push_back(static_cast<CharT>(cp));
            }
            else
            {
                prevCased = IsAlphaCp(cp);
                result.append(ch.begin(), ch.end());
            }
        }
        return Result(std::move(result));
    }
    static InternalValue Capitalize(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "capitalize", 0, 0);
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
        std::function<bool(View)> isStripped = [](View ch) { return unicode::IsSpace(CodePointOf(ch)); };
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
            ++b;
        while (right && e > b && isStripped(chars[e - 1]))
            --e;
        return Result(Join(chars, b, e));
    }
    static InternalValue Strip(View self, const CallParams& params, RenderContext&) { return StripImpl(self, params, "strip", true, true); }
    static InternalValue Lstrip(View self, const CallParams& params, RenderContext&) { return StripImpl(self, params, "lstrip", true, false); }
    static InternalValue Rstrip(View self, const CallParams& params, RenderContext&) { return StripImpl(self, params, "rstrip", false, true); }

    static InternalValue MakeList(std::vector<Str>&& parts)
    {
        InternalValueList items;
        items.reserve(parts.size());
        for (auto& p : parts)
            items.emplace_back(TargetString(std::move(p)));
        return ListAdapter::CreateAdapter(std::move(items));
    }

    static InternalValue SplitImpl(View self, const CallParams& params, const char* name, bool fromRight)
    {
        CheckArgs(params, name, 0, 2, { "sep", "maxsplit" });
        const auto* sepArg = ArgOrNone(params, 0, "sep");
        const auto* maxArg = Arg(params, 1, "maxsplit");
        int64_t maxSplit = maxArg == nullptr ? -1 : IntArg(*maxArg, name);
        std::vector<Str> parts;

        if (sepArg == nullptr)
        {
            // Runs of whitespace separate; leading and trailing whitespace is dropped
            auto chars = SplitCodePoints(self);
            auto isSpace = [&chars](size_t idx) { return unicode::IsSpace(CodePointOf(chars[idx])); };
            if (!fromRight)
            {
                size_t pos = 0;
                while (true)
                {
                    while (pos < chars.size() && isSpace(pos))
                        ++pos;
                    if (pos == chars.size())
                        break;
                    if (maxSplit >= 0 && static_cast<int64_t>(parts.size()) == maxSplit)
                    {
                        size_t end = chars.size();
                        parts.push_back(Join(chars, pos, end));
                        break;
                    }
                    size_t start = pos;
                    while (pos < chars.size() && !isSpace(pos))
                        ++pos;
                    parts.push_back(Join(chars, start, pos));
                }
            }
            else
            {
                size_t pos = chars.size();
                while (true)
                {
                    while (pos > 0 && isSpace(pos - 1))
                        --pos;
                    if (pos == 0)
                        break;
                    if (maxSplit >= 0 && static_cast<int64_t>(parts.size()) == maxSplit)
                    {
                        parts.push_back(Join(chars, 0, pos));
                        break;
                    }
                    size_t end = pos;
                    while (pos > 0 && !isSpace(pos - 1))
                        --pos;
                    parts.push_back(Join(chars, pos, end));
                }
                std::reverse(parts.begin(), parts.end());
            }
            return MakeList(std::move(parts));
        }

        auto sep = StrArg(self, *sepArg, name);
        if (sep.empty())
            Raise("empty separator");
        if (!fromRight)
        {
            size_t start = 0;
            while (maxSplit < 0 || static_cast<int64_t>(parts.size()) < maxSplit)
            {
                auto p = self.find(sep, start);
                if (p == View::npos)
                    break;
                parts.emplace_back(self.substr(start, p - start));
                start = p + sep.size();
            }
            parts.emplace_back(self.substr(start));
        }
        else
        {
            size_t end = self.size();
            while (maxSplit < 0 || static_cast<int64_t>(parts.size()) < maxSplit)
            {
                if (end < sep.size())
                    break;
                auto p = self.substr(0, end).rfind(sep);
                if (p == View::npos)
                    break;
                parts.emplace_back(self.substr(p + sep.size(), end - p - sep.size()));
                end = p;
            }
            parts.emplace_back(self.substr(0, end));
            std::reverse(parts.begin(), parts.end());
        }
        return MakeList(std::move(parts));
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
            auto cp = CodePointOf(chars[pos]);
            bool isBreak = (cp >= 0x0A && cp <= 0x0D) || (cp >= 0x1C && cp <= 0x1E) || cp == 0x85 || cp == 0x2028 || cp == 0x2029;
            if (!isBreak)
                continue;
            size_t end = pos + 1;
            if (cp == '\r' && end < chars.size() && CodePointOf(chars[end]) == '\n')
                ++end;
            parts.push_back(Join(chars, start, keepEnds ? end : pos));
            start = end;
            pos = end - 1;
        }
        if (start < chars.size())
            parts.push_back(Join(chars, start, chars.size()));
        return MakeList(std::move(parts));
    }

    static InternalValue JoinMethod(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "join", 1, 1);
        bool isConverted = false;
        auto list = ConvertToList(params.posParams[0], isConverted, false);
        if (!isConverted)
            Raise("can only join an iterable");
        Str result;
        size_t idx = 0;
        for (const auto& item : list)
        {
            auto str = GetAsSameString(self, item);
            if (!str || !IsStringValue(item))
                Raise(fmt::format("sequence item {}: expected str instance, {} found", idx, TypeName(item)));
            if (idx++ != 0)
                result.append(self.begin(), self.end());
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
        int64_t count = countArg == nullptr ? -1 : IntArg(*countArg, "replace");
        Str result;
        if (oldStr.empty())
        {
            // An empty pattern matches before every character and at the end
            auto chars = SplitCodePoints(self);
            for (size_t n = 0; n <= chars.size(); ++n)
            {
                if (count < 0 || static_cast<int64_t>(n) < count)
                    result += newStr;
                if (n < chars.size())
                    result.append(chars[n].begin(), chars[n].end());
            }
            return Result(std::move(result));
        }
        size_t start = 0;
        for (int64_t done = 0; count < 0 || done < count; ++done)
        {
            auto p = self.find(oldStr, start);
            if (p == View::npos)
                break;
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
        if (startChar != nullptr)
            *startChar = start;
        if (end < start)
            end = start;
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
                return InternalValue(false);
        }
        auto window = Window(self, chars, params, 1, name);
        auto matches = [&](const InternalValue& affixVal) {
            auto affix = GetAsSameString(self, affixVal);
            if (!affix || !IsStringValue(affixVal))
                Raise(std::string(name) + " first arg must be str or a tuple of str, not " + TypeName(affixVal));
            if (affix->size() > window.size())
                return false;
            return atStart ? window.substr(0, affix->size()) == View(*affix) : window.substr(window.size() - affix->size()) == View(*affix);
        };
        const auto& affixes = params.posParams[0];
        if (const auto* list = GetIf<ListAdapter>(&affixes))
        {
            if (!list->IsTuple())
                Raise(std::string(name) + " first arg must be str or a tuple of str, not list");
            for (const auto& item : *list)
            {
                if (matches(item))
                    return InternalValue(true);
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
                return -1;
        }
        size_t startChar = 0;
        auto window = Window(self, chars, params, 1, name, &startChar);
        if (sub.empty())
            return static_cast<int64_t>(fromRight ? CharIndex(self, chars, static_cast<size_t>(window.data() - self.data()) + window.size()) : startChar);
        auto p = fromRight ? window.rfind(sub) : window.find(sub);
        if (p == View::npos)
            return -1;
        return static_cast<int64_t>(CharIndex(self, chars, static_cast<size_t>(window.data() - self.data()) + p));
    }
    static InternalValue Find(View self, const CallParams& params, RenderContext&) { return FindImpl(self, params, "find", false); }
    static InternalValue Rfind(View self, const CallParams& params, RenderContext&) { return FindImpl(self, params, "rfind", true); }
    static InternalValue Index(View self, const CallParams& params, RenderContext&)
    {
        auto result = FindImpl(self, params, "index", false);
        if (result < 0)
            Raise("substring not found");
        return result;
    }
    static InternalValue Rindex(View self, const CallParams& params, RenderContext&)
    {
        auto result = FindImpl(self, params, "rindex", true);
        if (result < 0)
            Raise("substring not found");
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
                return static_cast<int64_t>(0);
        }
        auto window = Window(self, chars, params, 1, "count");
        if (sub.empty())
            return static_cast<int64_t>(CodePointCount(window) + 1);
        int64_t result = 0;
        for (auto p = window.find(sub); p != View::npos; p = window.find(sub, p + sub.size()))
            ++result;
        return result;
    }

    template<typename Pred>
    static InternalValue AllChars(View self, const CallParams& params, const char* name, Pred&& pred)
    {
        CheckArgs(params, name, 0, 0);
        auto chars = SplitCodePoints(self);
        if (chars.empty())
            return InternalValue(false);
        return InternalValue(std::all_of(chars.begin(), chars.end(), [&pred](View ch) { return pred(CodePointOf(ch)); }));
    }
    static InternalValue Isdigit(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isdigit", unicode::IsDecimal); }
    static InternalValue Isdecimal(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isdecimal", unicode::IsDecimal); }
    static InternalValue Isnumeric(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isnumeric", unicode::IsDecimal); }
    static InternalValue Isalpha(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isalpha", IsAlphaCp); }
    static InternalValue Isalnum(View self, const CallParams& params, RenderContext&)
    {
        return AllChars(self, params, "isalnum", [](uint32_t cp) { return unicode::IsWordChar(cp) && cp != '_'; });
    }
    static InternalValue Isspace(View self, const CallParams& params, RenderContext&) { return AllChars(self, params, "isspace", unicode::IsSpace); }

    static InternalValue CaseCheck(View self, const CallParams& params, const char* name, bool upper)
    {
        CheckArgs(params, name, 0, 0);
        bool hasCased = false;
        for (auto ch : SplitCodePoints(self))
        {
            auto cp = CodePointOf(ch);
            if (upper ? IsAsciiLower(cp) : IsAsciiUpper(cp))
                return InternalValue(false);
            hasCased = hasCased || IsAsciiUpper(cp) || IsAsciiLower(cp);
        }
        return InternalValue(hasCased);
    }
    static InternalValue Isupper(View self, const CallParams& params, RenderContext&) { return CaseCheck(self, params, "isupper", true); }
    static InternalValue Islower(View self, const CallParams& params, RenderContext&) { return CaseCheck(self, params, "islower", false); }

    static InternalValue Zfill(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "zfill", 1, 1);
        auto width = WidthArg(IntArg(params.posParams[0], "zfill"));
        auto len = static_cast<int64_t>(CodePointCount(self));
        if (width <= len)
            return Result(Str(self.begin(), self.end()));
        Str result(static_cast<size_t>(width - len), static_cast<CharT>('0'));
        if (!self.empty() && (self[0] == '+' || self[0] == '-'))
            return Result(Str(1, self[0]) + result + Str(self.begin() + 1, self.end()));
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
                Raise(std::string("The fill character must be exactly one character long"));
        }
        auto len = static_cast<int64_t>(CodePointCount(self));
        if (width <= len)
            return Result(Str(self.begin(), self.end()));
        auto pad = width - len;
        // Python's center() puts the odd character on the left when the width is odd
        int64_t left = pad / 2 + (pad & width & 1);
        if (align != 0)
            left = align < 0 ? 0 : pad;
        Str result;
        for (int64_t n = 0; n < left; ++n)
            result += fill;
        result.append(self.begin(), self.end());
        for (int64_t n = left; n < pad; ++n)
            result += fill;
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
            Raise("empty separator");
        auto p = fromRight ? self.rfind(sep) : self.find(sep);
        Str whole(self.begin(), self.end());
        if (p == View::npos)
            return fromRight ? MakeTuple({ TargetString(Str()), TargetString(Str()), TargetString(whole) })
                             : MakeTuple({ TargetString(whole), TargetString(Str()), TargetString(Str()) });
        return MakeTuple({ TargetString(Str(self.substr(0, p))), TargetString(sep), TargetString(Str(self.substr(p + sep.size()))) });
    }
    static InternalValue Partition(View self, const CallParams& params, RenderContext&) { return PartitionImpl(self, params, "partition", false); }
    static InternalValue Rpartition(View self, const CallParams& params, RenderContext&) { return PartitionImpl(self, params, "rpartition", true); }

    static InternalValue Removeprefix(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "removeprefix", 1, 1);
        auto prefix = StrArg(self, params.posParams[0], "removeprefix");
        if (self.substr(0, prefix.size()) == View(prefix))
            self.remove_prefix(prefix.size());
        return Result(Str(self.begin(), self.end()));
    }
    static InternalValue Removesuffix(View self, const CallParams& params, RenderContext&)
    {
        CheckArgs(params, "removesuffix", 1, 1);
        auto suffix = StrArg(self, params.posParams[0], "removesuffix");
        if (!suffix.empty() && self.size() >= suffix.size() && self.substr(self.size() - suffix.size()) == View(suffix))
            self.remove_suffix(suffix.size());
        return Result(Str(self.begin(), self.end()));
    }

    static Str ToStr(const InternalValue& val, bool asRepr)
    {
        Str result;
        Apply<visitors::ValueRenderer<CharT>>(val, result, asRepr);
        return result;
    }

    static Str Ascii(const std::string& str) { return Str(str.begin(), str.end()); }

    // format(value, spec) for the subset of Python's format mini-language that templates use:
    // [[fill]align][sign][#][0][width][,|_][.precision][type]
    static Str FormatValue(const InternalValue& val, View spec)
    {
        if (spec.empty())
            return ToStr(val, false);

        auto chars = SplitCodePoints(spec);
        size_t pos = 0;
        Str fill(1, static_cast<CharT>(' '));
        CharT align = 0;
        auto isAlign = [](uint32_t cp) { return cp == '<' || cp == '>' || cp == '^' || cp == '='; };
        if (chars.size() >= 2 && isAlign(CodePointOf(chars[1])))
        {
            fill = Str(chars[0].begin(), chars[0].end());
            align = chars[1][0];
            pos = 2;
        }
        else if (!chars.empty() && isAlign(CodePointOf(chars[0])))
        {
            align = chars[0][0];
            pos = 1;
        }
        auto peek = [&]() -> uint32_t { return pos < chars.size() ? CodePointOf(chars[pos]) : 0; };
        uint32_t sign = 0;
        if (peek() == '+' || peek() == '-' || peek() == ' ')
            sign = CodePointOf(chars[pos++]);
        bool alternate = false;
        if (peek() == '#')
        {
            alternate = true;
            ++pos;
        }
        if (peek() == '0')
        {
            if (align == 0)
            {
                fill = Str(1, static_cast<CharT>('0'));
                align = '=';
            }
            ++pos;
        }
        auto readNumber = [&]() {
            int64_t value = 0;
            while (peek() >= '0' && peek() <= '9')
                value = WidthArg(value * 10 + (CodePointOf(chars[pos++]) - '0'));
            return value;
        };
        int64_t width = readNumber();
        uint32_t grouping = 0;
        if (peek() == ',' || peek() == '_')
            grouping = CodePointOf(chars[pos++]);
        int64_t precision = -1;
        if (peek() == '.')
        {
            ++pos;
            precision = 0;
            if (!(peek() >= '0' && peek() <= '9'))
                Raise("Format specifier missing precision");
            precision = readNumber();
        }
        uint32_t type = 0;
        if (pos < chars.size())
            type = CodePointOf(chars[pos++]);
        if (pos != chars.size())
            Raise("Invalid format specifier");

        Str body;
        Str signStr;
        bool numeric = false;
        const auto* intVal = GetIf<int64_t>(&val);
        const auto* boolVal = GetIf<bool>(&val);
        const auto* dblVal = GetIf<double>(&val);
        int64_t intValue = 0;
        if (intVal != nullptr)
            intValue = *intVal;
        else if (boolVal != nullptr)
            intValue = *boolVal ? 1 : 0;
        bool isInt = intVal != nullptr || (boolVal != nullptr && type != 0 && type != 's');
        if (IsStringValue(val) || (boolVal != nullptr && type == 0))
        {
            if (type != 0 && type != 's')
                Raise(fmt::format("Unknown format code '{}' for object of type '{}'", static_cast<char>(type), TypeName(val)));
            if (sign != 0 || alternate || grouping != 0 || align == '=')
                Raise("Invalid format specifier for a string");
            body = ToStr(val, false);
            if (precision >= 0)
            {
                auto bodyChars = SplitCodePoints(View(body));
                if (static_cast<size_t>(precision) < bodyChars.size())
                    body = Join(bodyChars, 0, static_cast<size_t>(precision));
            }
        }
        else if (isInt && (type == 0 || type == 'd' || type == 'b' || type == 'o' || type == 'x' || type == 'X' || type == 'n' || type == 'c'))
        {
            numeric = true;
            if (precision >= 0)
                Raise("Precision not allowed in integer format specifier");
            if (type == 'c')
            {
                body = ToStr(InternalValue(TargetString(Str(1, static_cast<CharT>(intValue)))), false);
                numeric = false;
            }
            else
            {
                uint64_t magnitude = intValue < 0 ? 0 - static_cast<uint64_t>(intValue) : static_cast<uint64_t>(intValue);
                int base = 10;
                if (type == 'b')
                    base = 2;
                else if (type == 'o')
                    base = 8;
                else if (type == 'x' || type == 'X')
                    base = 16;
                std::string digits;
                do
                {
                    auto d = static_cast<int>(magnitude % static_cast<uint64_t>(base));
                    digits.push_back(static_cast<char>(d < 10 ? '0' + d : (type == 'X' ? 'A' : 'a') + d - 10));
                    magnitude /= static_cast<uint64_t>(base);
                } while (magnitude != 0);
                if (grouping != 0)
                {
                    size_t groupSize = base == 10 ? 3 : 4;
                    std::string grouped;
                    for (size_t n = 0; n < digits.size(); ++n)
                    {
                        if (n != 0 && n % groupSize == 0)
                            grouped.push_back(static_cast<char>(grouping));
                        grouped.push_back(digits[n]);
                    }
                    digits = grouped;
                }
                std::reverse(digits.begin(), digits.end());
                if (alternate && base != 10)
                    digits = std::string(1, '0') + static_cast<char>(type) + digits;
                body = Ascii(digits);
                if (intValue < 0)
                    signStr = Ascii("-");
            }
        }
        else if (isInt || dblVal != nullptr)
        {
            numeric = true;
            double value = dblVal != nullptr ? *dblVal : static_cast<double>(intValue);
            if (type != 0 && type != 'e' && type != 'E' && type != 'f' && type != 'F' && type != 'g' && type != 'G' && type != '%' && type != 'n')
                Raise(fmt::format("Unknown format code '{}' for object of type '{}'", static_cast<char>(type), TypeName(val)));
            bool negative = std::signbit(value) && !std::isnan(value);
            double magnitude = std::fabs(value);
            std::string digits;
            int prec = precision < 0 ? 6 : static_cast<int>(precision);
            if (std::isinf(magnitude) || std::isnan(magnitude))
                digits = std::isnan(magnitude) ? "nan" : "inf";
            else if (type == 'f' || type == 'F')
                digits = fmt::format("{:.{}f}", magnitude, prec);
            else if (type == '%')
                digits = fmt::format("{:.{}f}", magnitude * 100, prec) + "%";
            else if (type == 'e' || type == 'E')
                digits = fmt::format("{:.{}e}", magnitude, prec);
            else if (type == 0 && precision < 0)
                digits = visitors::FormatPythonFloat(magnitude);
            else
            {
                // 'g' (and no type with a precision): significant digits, trailing zeros dropped
                digits = fmt::format("{:.{}g}", magnitude, std::max(prec, 1));
                if (type == 0 && digits.find_first_of(".e") == std::string::npos)
                    digits += ".0";
            }
            if (type == 'E' || type == 'F' || type == 'G')
                std::transform(digits.begin(), digits.end(), digits.begin(), [](char ch) { return static_cast<char>(std::toupper(static_cast<unsigned char>(ch))); });
            if (grouping != 0)
            {
                auto intEnd = digits.find_first_not_of("0123456789");
                if (intEnd == std::string::npos)
                    intEnd = digits.size();
                std::string grouped;
                for (size_t n = 0; n < intEnd; ++n)
                {
                    if (n != 0 && (intEnd - n) % 3 == 0)
                        grouped.push_back(static_cast<char>(grouping));
                    grouped.push_back(digits[n]);
                }
                digits = grouped + digits.substr(intEnd);
            }
            body = Ascii(digits);
            if (negative)
                signStr = Ascii("-");
        }
        else
        {
            Raise("unsupported format string passed to " + TypeName(val) + ".__format__");
        }

        if (numeric && signStr.empty() && (sign == '+' || sign == ' '))
            signStr = Str(1, static_cast<CharT>(sign));

        auto len = static_cast<int64_t>(CodePointCount(View(signStr)) + CodePointCount(View(body)));
        if (width <= len)
            return signStr + body;
        auto pad = width - len;
        if (align == 0)
            align = numeric ? '>' : '<';
        Str padding;
        auto makePad = [&fill](int64_t n) {
            Str result;
            for (int64_t i = 0; i < n; ++i)
                result += fill;
            return result;
        };
        switch (align)
        {
        case '<':
            return signStr + body + makePad(pad);
        case '^':
            return makePad(pad / 2) + signStr + body + makePad(pad - pad / 2);
        case '=':
            return signStr + makePad(pad) + body;
        default:
            return makePad(pad) + signStr + body;
        }
    }

    // str.format: {}, {0}, {name}, attribute and index lookups {0.x} {a[k]}, !r/!s and a
    // format spec. Nested replacement fields inside a spec are not supported.
    static InternalValue Format(View self, const CallParams& params, RenderContext& context)
    {
        Str result;
        size_t autoIdx = 0;
        bool usedAuto = false;
        bool usedManual = false;
        auto toNarrow = [](View str) { return ConvertString<std::string>(str); };
        for (size_t pos = 0; pos < self.size(); ++pos)
        {
            auto ch = self[pos];
            if (ch == '}')
            {
                if (pos + 1 < self.size() && self[pos + 1] == '}')
                {
                    result.push_back(ch);
                    ++pos;
                    continue;
                }
                Raise("Single '}' encountered in format string");
            }
            if (ch != '{')
            {
                result.push_back(ch);
                continue;
            }
            if (pos + 1 < self.size() && self[pos + 1] == '{')
            {
                result.push_back(ch);
                ++pos;
                continue;
            }
            auto close = self.find('}', pos + 1);
            if (close == View::npos)
                Raise("Single '{' encountered in format string");
            auto field = self.substr(pos + 1, close - pos - 1);
            if (field.find('{') != View::npos)
                Raise("Nested replacement fields are not supported");
            pos = close;

            View spec;
            auto colon = field.find(':');
            if (colon != View::npos)
            {
                spec = field.substr(colon + 1);
                field = field.substr(0, colon);
            }
            CharT conversion = 0;
            auto bang = field.find('!');
            if (bang != View::npos)
            {
                if (bang + 2 != field.size())
                    Raise("expected ':' after conversion specifier");
                conversion = field[bang + 1];
                field = field.substr(0, bang);
            }

            // The first name, then .attr and [key] parts
            auto nameEnd = field.find_first_of(Ascii(".["));
            auto first = field.substr(0, nameEnd == View::npos ? field.size() : nameEnd);
            auto rest = nameEnd == View::npos ? View() : field.substr(nameEnd);
            InternalValue value;
            bool isNumber = !first.empty() && std::all_of(first.begin(), first.end(), [](CharT c) { return c >= '0' && c <= '9'; });
            if (first.empty() || isNumber)
            {
                size_t idx = 0;
                if (first.empty())
                {
                    if (usedManual)
                        Raise("cannot switch from manual field specification to automatic field numbering");
                    usedAuto = true;
                    idx = autoIdx++;
                }
                else
                {
                    if (usedAuto)
                        Raise("cannot switch from automatic field numbering to manual field specification");
                    usedManual = true;
                    idx = static_cast<size_t>(std::stoull(toNarrow(first)));
                }
                if (idx >= params.posParams.size())
                    Raise(fmt::format("Replacement index {} out of range for positional args tuple", idx));
                value = params.posParams[idx];
            }
            else
            {
                auto p = params.kwParams.find(toNarrow(first));
                if (p == params.kwParams.end())
                    Raise("'" + toNarrow(first) + "'");
                value = p->second;
            }
            while (!rest.empty())
            {
                if (rest[0] == '.')
                {
                    auto end = rest.find_first_of(Ascii(".["), 1);
                    auto attr = toNarrow(rest.substr(1, end == View::npos ? View::npos : end - 1));
                    if (attr.empty())
                        Raise("Empty attribute in format string");
                    value = GetAttr(value, attr, &context);
                    rest = end == View::npos ? View() : rest.substr(end);
                }
                else
                {
                    auto end = rest.find(']');
                    if (end == View::npos)
                        Raise("Missing ']' in format string");
                    auto key = rest.substr(1, end - 1);
                    bool keyIsNumber = !key.empty() && std::all_of(key.begin(), key.end(), [](CharT c) { return c >= '0' && c <= '9'; });
                    InternalValue keyVal = keyIsNumber ? InternalValue(static_cast<int64_t>(std::stoll(toNarrow(key)))) : InternalValue(toNarrow(key));
                    value = GetItem(value, keyVal, &context);
                    rest = rest.substr(end + 1);
                    if (!rest.empty() && rest[0] != '.' && rest[0] != '[')
                        Raise("Only '.' or '[' may follow ']' in format field specifier");
                }
            }
            if (conversion == 'r' || conversion == 's' || conversion == 'a')
                value = TargetString(ToStr(value, conversion != 's'));
            else if (conversion != 0)
                Raise("Unknown conversion specifier");
            result += FormatValue(value, spec);
        }
        return Result(std::move(result));
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
JINJA2CPP_STR_METHOD(Format)
JINJA2CPP_STR_METHOD(Isdigit)
JINJA2CPP_STR_METHOD(Isdecimal)
JINJA2CPP_STR_METHOD(Isnumeric)
JINJA2CPP_STR_METHOD(Isalpha)
JINJA2CPP_STR_METHOD(Isalnum)
JINJA2CPP_STR_METHOD(Isspace)
JINJA2CPP_STR_METHOD(Isupper)
JINJA2CPP_STR_METHOD(Islower)
JINJA2CPP_STR_METHOD(Zfill)
JINJA2CPP_STR_METHOD(Center)
JINJA2CPP_STR_METHOD(Ljust)
JINJA2CPP_STR_METHOD(Rjust)
JINJA2CPP_STR_METHOD(Partition)
JINJA2CPP_STR_METHOD(Rpartition)
JINJA2CPP_STR_METHOD(Removeprefix)
JINJA2CPP_STR_METHOD(Removesuffix)

#undef JINJA2CPP_STR_METHOD

const MethodInfo StrMethods[] = {
    { "upper", StrUpper, false },
    { "lower", StrLower, false },
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
    { "isalpha", StrIsalpha, false },
    { "isalnum", StrIsalnum, false },
    { "isspace", StrIsspace, false },
    { "isupper", StrIsupper, false },
    { "islower", StrIslower, false },
    { "zfill", StrZfill, false },
    { "center", StrCenter, false },
    { "ljust", StrLjust, false },
    { "rjust", StrRjust, false },
    { "partition", StrPartition, false },
    { "rpartition", StrRpartition, false },
    { "removeprefix", StrRemoveprefix, false },
    { "removesuffix", StrRemovesuffix, false },
};

// ---------------------------------------------------------------------------------------
// list and tuple

const ListAdapter& ListOf(const InternalValue& self)
{
    return *GetIf<ListAdapter>(&self);
}

InternalValueList& MutableItems(const InternalValue& self)
{
    auto* items = ListOf(self).GetMutableItems();
    // The call path makes the receiver mutable first (MakeMutable)
    if (items == nullptr)
        Raise("this list cannot be changed");
    return *items;
}

// Storing a container in itself would make a reference cycle that shared ownership never
// frees. Python allows it; Jinja2C++ refuses (a deliberate divergence, docs/tasks/0020).
bool Reaches(const InternalValue& val, const void* target, std::unordered_set<const void*>& visited)
{
    const void* storage = nullptr;
    if (const auto* list = GetIf<ListAdapter>(&val))
        storage = list->GetMutableItems();
    else if (const auto* map = GetIf<MapAdapter>(&val))
        storage = map->GetMutableItems();
    else if (const auto* callable = GetIf<Callable>(&val))
    {
        const auto& attrs = callable->GetAttributes();
        if (!attrs)
            return false;
        auto p = attrs->find("__self__");
        return p != attrs->end() && Reaches(p->second, target, visited);
    }
    // Only containers the template owns can hold one another
    if (storage == nullptr)
        return false;
    if (storage == target)
        return true;
    if (!visited.insert(storage).second)
        return false;
    if (const auto* list = GetIf<ListAdapter>(&val))
    {
        for (auto& item : *list->GetMutableItems())
        {
            if (Reaches(item, target, visited))
                return true;
        }
        return false;
    }
    for (auto& item : *GetIf<MapAdapter>(&val)->GetMutableItems())
    {
        if (Reaches(item.second, target, visited))
            return true;
    }
    return false;
}

void CheckNoCycle(const void* storage, const InternalValue& item)
{
    std::unordered_set<const void*> visited;
    if (Reaches(item, storage, visited))
        Raise("a list or dict cannot contain itself");
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
            return static_cast<int64_t>(n);
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
            ++result;
    }
    return result;
}

InternalValue ListAppend(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "append", 1, 1);
    auto& items = MutableItems(self);
    CheckNoCycle(&items, params.posParams[0]);
    items.push_back(params.posParams[0]);
    return EmptyValue();
}

InternalValue ListExtend(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "extend", 1, 1);
    bool isConverted = false;
    auto other = ConvertToList(params.posParams[0], isConverted);
    if (!isConverted)
        Raise("'" + TypeName(params.posParams[0]) + "' object is not iterable");
    // A snapshot first: l.extend(l) doubles the list, as in Python
    auto newItems = other.ToValueList();
    auto& items = MutableItems(self);
    for (auto& item : newItems)
        CheckNoCycle(&items, item);
    items.insert(items.end(), newItems.begin(), newItems.end());
    return EmptyValue();
}

InternalValue ListInsert(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "insert", 2, 2);
    auto& items = MutableItems(self);
    auto idx = SliceIndex(&params.posParams[0], items.size(), 0, "insert");
    CheckNoCycle(&items, params.posParams[1]);
    items.insert(items.begin() + static_cast<std::ptrdiff_t>(idx), params.posParams[1]);
    return EmptyValue();
}

InternalValue ListPop(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "pop", 0, 1);
    auto& items = MutableItems(self);
    if (items.empty())
        Raise("pop from empty list");
    int64_t idx = params.posParams.empty() ? -1 : IntArg(params.posParams[0], "pop");
    if (idx < 0)
        idx += static_cast<int64_t>(items.size());
    if (idx < 0 || idx >= static_cast<int64_t>(items.size()))
        Raise("pop index out of range");
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
        Raise("list.remove(x): x not in list");
    items.erase(p);
    return EmptyValue();
}

InternalValue ListReverse(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "reverse", 0, 0);
    auto& items = MutableItems(self);
    std::reverse(items.begin(), items.end());
    return EmptyValue();
}

InternalValue ListClear(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "clear", 0, 0);
    MutableItems(self).clear();
    return EmptyValue();
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
    { "clear", ListClear, true },
    { "copy", ListCopy, false },
};

const MethodInfo TupleMethods[] = {
    { "index", ListIndex, false },
    { "count", ListCount, false },
};

// ---------------------------------------------------------------------------------------
// dict

const MapAdapter& MapOf(const InternalValue& self)
{
    return *GetIf<MapAdapter>(&self);
}

InternalDict& MutableDict(const InternalValue& self)
{
    auto* items = MapOf(self).GetMutableItems();
    if (items == nullptr)
        Raise("this dict cannot be changed");
    return *items;
}

// The keys in iteration order: insertion order for the dicts the template owns
std::vector<std::string> KeysOf(const MapAdapter& map)
{
    if (auto* items = map.GetMutableItems())
    {
        std::vector<std::string> keys;
        keys.reserve(items->size());
        for (auto& item : *items)
            keys.push_back(item.first);
        return keys;
    }
    return map.GetKeys();
}

InternalValue DictKeys(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "keys", 0, 0);
    InternalValueList result;
    for (auto& key : KeysOf(MapOf(self)))
        result.emplace_back(key);
    return ListAdapter::CreateAdapter(std::move(result));
}

InternalValue DictValues(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "values", 0, 0);
    const auto& map = MapOf(self);
    InternalValueList result;
    for (auto& key : KeysOf(map))
        result.push_back(map.GetValueByName(key));
    InternalValue list = ListAdapter::CreateAdapter(std::move(result));
    if (self.ShouldExtendLifetime())
        list.SetParentData(self);
    return list;
}

InternalValue DictItems(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "items", 0, 0);
    const auto& map = MapOf(self);
    InternalValueList result;
    for (auto& key : KeysOf(map))
        result.push_back(MakeTuple({ InternalValue(key), map.GetValueByName(key) }));
    InternalValue list = ListAdapter::CreateAdapter(std::move(result));
    if (self.ShouldExtendLifetime())
        list.SetParentData(self);
    return list;
}

InternalValue DictGet(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "get", 1, 2);
    const auto& map = MapOf(self);
    auto key = KeyString(params.posParams[0]);
    if (map.HasValue(key))
        return map.GetValueByName(key);
    return params.posParams.size() > 1 ? params.posParams[1] : InternalValue(EmptyValue());
}

InternalValue DictSetdefault(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "setdefault", 1, 2);
    auto& items = MutableDict(self);
    auto key = KeyString(params.posParams[0]);
    auto p = items.find(key);
    if (p != items.end())
        return p->second;
    InternalValue value = params.posParams.size() > 1 ? params.posParams[1] : InternalValue(EmptyValue());
    CheckNoCycle(&items, value);
    items[key] = value;
    return value;
}

InternalValue DictUpdate(const InternalValue& self, const CallParams& params, RenderContext&)
{
    if (params.posParams.size() > 1)
        Raise(fmt::format("update expected at most 1 argument, got {}", params.posParams.size()));
    // Collect first: d.update(d) and failing pairs leave d as it was
    std::vector<std::pair<std::string, InternalValue>> updates;
    if (!params.posParams.empty())
    {
        const auto& other = params.posParams[0];
        if (const auto* otherMap = GetIf<MapAdapter>(&other))
        {
            for (auto& key : KeysOf(*otherMap))
                updates.emplace_back(key, otherMap->GetValueByName(key));
        }
        else
        {
            bool isConverted = false;
            auto pairs = ConvertToList(other, isConverted);
            if (!isConverted || IsStringValue(other))
                Raise("'" + TypeName(other) + "' object is not iterable");
            for (const auto& pairVal : pairs)
            {
                bool isPair = false;
                auto pairList = ConvertToList(pairVal, isPair);
                auto pair = isPair ? pairList.ToValueList() : InternalValueList();
                if (!isPair || pair.size() != 2)
                    Raise("dictionary update sequence element has wrong length; 2 is required");
                updates.emplace_back(KeyString(pair[0]), pair[1]);
            }
        }
    }
    for (const auto& kw : params.kwParams)
        updates.emplace_back(kw.first, kw.second);
    auto& items = MutableDict(self);
    for (auto& u : updates)
        CheckNoCycle(&items, u.second);
    for (auto& u : updates)
        items[u.first] = std::move(u.second);
    return EmptyValue();
}

InternalValue DictPop(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "pop", 1, 2);
    auto& items = MutableDict(self);
    auto key = KeyString(params.posParams[0]);
    auto p = items.find(key);
    if (p == items.end())
    {
        if (params.posParams.size() > 1)
            return params.posParams[1];
        Raise("KeyError: '" + key + "'");
    }
    auto result = p->second;
    items.erase(p);
    return result;
}

InternalValue DictPopitem(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "popitem", 0, 0);
    auto& items = MutableDict(self);
    if (items.empty())
        Raise("popitem(): dictionary is empty");
    auto last = std::prev(items.end());
    auto result = MakeTuple({ InternalValue(last->first), last->second });
    items.erase(last);
    return result;
}

InternalValue DictCopy(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "copy", 0, 0);
    const auto& map = MapOf(self);
    InternalDict result;
    for (auto& key : KeysOf(map))
        result[key] = map.GetValueByName(key);
    return CreateMapAdapter(std::move(result));
}

InternalValue DictClear(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "clear", 0, 0);
    MutableDict(self).clear();
    return EmptyValue();
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
    auto value = IntArg(self, "bit_length");
    uint64_t magnitude = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    int64_t bits = 0;
    for (; magnitude != 0; magnitude >>= 1)
        ++bits;
    return bits;
}

InternalValue FloatIsInteger(const InternalValue& self, const CallParams& params, RenderContext&)
{
    CheckArgs(params, "is_integer", 0, 0);
    auto value = *GetIf<double>(&self);
    return InternalValue(std::isfinite(value) && std::floor(value) == value);
}

const MethodInfo IntMethods[] = {
    { "bit_length", IntBitLength, false },
};

const MethodInfo FloatMethods[] = {
    { "is_integer", FloatIsInteger, false },
};

template<size_t N>
const MethodInfo* FindIn(const MethodInfo (&table)[N], std::string_view name)
{
    for (auto& m : table)
    {
        if (name == m.name)
            return &m;
    }
    return nullptr;
}

const MethodInfo* FindMethodByKind(const InternalValue& self, std::string_view name)
{
    const auto& data = self.GetData();
    if (IsStringValue(self))
        return FindIn(StrMethods, name);
    if (const auto* list = std::get_if<ListAdapter>(&data))
        return list->IsTuple() || list->GetRangeInfo() != nullptr ? FindIn(TupleMethods, name) : FindIn(ListMethods, name);
    if (const auto* map = std::get_if<MapAdapter>(&data))
        return map->GetAttrPolicy() == MapAttrPolicy::KeysOnly ? nullptr : FindIn(DictMethods, name);
    if (std::get_if<int64_t>(&data) != nullptr || std::get_if<bool>(&data) != nullptr)
        return FindIn(IntMethods, name);
    if (std::get_if<double>(&data) != nullptr)
        return FindIn(FloatMethods, name);
    return nullptr;
}
} // namespace

bool IsMethodName(std::string_view name)
{
    return FindIn(StrMethods, name) || FindIn(ListMethods, name) || FindIn(DictMethods, name) || FindIn(IntMethods, name) || FindIn(FloatMethods, name);
}

const MethodInfo* FindMethod(const InternalValue& self, std::string_view name)
{
    return FindMethodByKind(self, name);
}

InternalValue MakeBoundMethod(const InternalValue& self, const MethodInfo& method)
{
    auto fn = method.invoke;
    Callable result(Callable::GlobalFunc, [self, fn](const CallParams& params, RenderContext& context) { return fn(self, params, context); });
    result.SetAttributes(std::make_shared<const InternalValueMap>(InternalValueMap{ { "__self__", self } }));
    return result;
}

InternalValue GetAttr(const InternalValue& obj, const std::string& name, RenderContext* context)
{
    if (const auto* method = FindMethod(obj, name))
    {
        const auto* map = GetIf<MapAdapter>(&obj);
        if (map == nullptr || map->GetAttrPolicy() == MapAttrPolicy::MethodsFirst || !map->HasValue(name))
            return MakeBoundMethod(obj, *method);
    }
    return Subscript(obj, name, context);
}

InternalValue GetItem(const InternalValue& obj, const InternalValue& key, RenderContext* context)
{
    auto result = Subscript(obj, key, context);
    if (result.IsUndefined() && IsStringValue(key))
    {
        if (const auto* method = FindMethod(obj, AsString(key)))
            return MakeBoundMethod(obj, *method);
    }
    return result;
}

bool IsMutatingName(std::string_view name)
{
    const auto* listMethod = FindIn(ListMethods, name);
    const auto* dictMethod = FindIn(DictMethods, name);
    return (listMethod != nullptr && listMethod->isMutating) || (dictMethod != nullptr && dictMethod->isMutating);
}

void StoreItem(const InternalValue& container, const InternalValue& key, InternalValue value)
{
    if (const auto* list = GetIf<ListAdapter>(&container))
    {
        auto* items = list->GetMutableItems();
        const auto* idxVal = GetIf<int64_t>(&key);
        if (items == nullptr || idxVal == nullptr)
            return;
        auto idx = *idxVal < 0 ? *idxVal + static_cast<int64_t>(items->size()) : *idxVal;
        if (idx >= 0 && idx < static_cast<int64_t>(items->size()))
            (*items)[static_cast<size_t>(idx)] = std::move(value);
    }
    else if (const auto* map = GetIf<MapAdapter>(&container))
    {
        auto* items = map->GetMutableItems();
        if (items != nullptr)
            (*items)[KeyString(key)] = std::move(value);
    }
}

bool IsContainer(const InternalValue& value)
{
    return GetIf<ListAdapter>(&value) != nullptr || GetIf<MapAdapter>(&value) != nullptr;
}

bool IsMutable(const InternalValue& value)
{
    if (const auto* list = GetIf<ListAdapter>(&value))
        return list->GetMutableItems() != nullptr;
    if (const auto* map = GetIf<MapAdapter>(&value))
        return map->GetMutableItems() != nullptr;
    return false;
}

InternalValue CopyContainer(const InternalValue& value)
{
    if (const auto* list = GetIf<ListAdapter>(&value))
    {
        auto copy = ListAdapter::CreateAdapter(list->ToValueList());
        if (list->IsTuple())
            copy.MarkAsTuple();
        return copy;
    }
    if (const auto* map = GetIf<MapAdapter>(&value))
    {
        InternalDict items;
        for (auto& key : KeysOf(*map))
            items[key] = map->GetValueByName(key);
        return CreateMapAdapter(std::move(items));
    }
    return value;
}

InternalValue MakeMutable(const InternalValue& value)
{
    if (IsMutable(value))
        return value;
    bool extendLifetime = value.ShouldExtendLifetime();
    if (const auto* list = GetIf<ListAdapter>(&value))
    {
        // Tuples and ranges are never changed in place: only their read-only methods exist
        if (list->IsTuple() || list->GetRangeInfo() != nullptr)
            return value;
        auto items = list->ToValueList();
        if (extendLifetime)
        {
            for (auto& item : items)
                item.SetParentData(value);
        }
        return ListAdapter::CreateAdapter(std::move(items));
    }
    if (const auto* map = GetIf<MapAdapter>(&value))
    {
        InternalDict items;
        for (auto& key : map->GetKeys())
        {
            auto item = map->GetValueByName(key);
            if (extendLifetime)
                item.SetParentData(value);
            items[key] = std::move(item);
        }
        return CreateMapAdapter(std::move(items));
    }
    return value;
}

void ThrowNoAttribute(const InternalValue& obj, const std::string& name)
{
    Raise("'" + TypeName(obj) + " object' has no attribute '" + name + "'");
}

} // namespace methods
} // namespace jinja2
