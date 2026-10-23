#include "big_int.h"
#include "expression_evaluator.h"
#include "filters.h"
#include "function_base.h"
#include "internal_value.h"
#include "markup.h"
#include "out_stream.h" // IWYU pragma: keep (GetStreamOnString returns an OutStream by value)
#include "python_format.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "string_builder.h"
#include "template_dict.h"
#include "undefined.h"
#include "unicode_printable.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{
inline namespace v2
{
namespace filters
{
struct PrettyPrinter : visitors::BaseVisitor<std::string>
{
    using BaseVisitor::operator();

    explicit PrettyPrinter(const RenderContext* context)
        : m_context(context)
    {
    }

    // pprint's _safe_key: `a < b` when Python can order the two keys, else by the name of
    // their types (str(type)), so that {1: 'a', 'b': 2} prints with the int first. Two keys
    // of one type that cannot be ordered (None and None) stay where they are, as Python
    // compares their ids
    static bool SafeKeyLess(const InternalValue& left, const InternalValue& right)
    {
        const auto isNumber = [](const InternalValue& val) {
            return val.Kind() == ValueKind::Int || val.Kind() == ValueKind::BigInt || val.Kind() == ValueKind::Double || val.Kind() == ValueKind::Bool;
        };
        const auto isSequence = [](const InternalValue& val) { return val.Kind() == ValueKind::List || val.Kind() == ValueKind::KeyValuePair; };
        if ((isNumber(left) && isNumber(right)) || (IsStringValue(left) && IsStringValue(right)) || (isSequence(left) && isSequence(right)))
        {
            try
            {
                return ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, BinaryExpression::LogicalLt, BinaryExpression::CaseSensitive));
            }
            catch (const std::runtime_error&) // NOLINT(bugprone-empty-catch): the fallback below is the handling
            {
                // Tuples with items Python cannot order: ordered by type like the rest
            }
        }
        return std::strcmp(Apply<visitors::PythonTypeNameGetter>(left), Apply<visitors::PythonTypeNameGetter>(right)) < 0;
    }

    // The entries in the order pprint prints them: by key. String keys sort by code point,
    // which is UTF-8 byte order
    static void SortEntriesByKey(std::vector<KeyValuePair>& entries)
    {
        if (!std::all_of(entries.begin(), entries.end(), [](const KeyValuePair& entry) { return IsStringValue(entry.key); }))
        {
            std::stable_sort(entries.begin(), entries.end(), [](const KeyValuePair& left, const KeyValuePair& right) { return SafeKeyLess(left.key, right.key); });
            return;
        }
        std::vector<std::pair<std::string, size_t>> order;
        order.reserve(entries.size());
        for (size_t idx = 0; idx != entries.size(); ++idx)
        {
            order.emplace_back(AsString(entries[idx].key), idx);
        }
        std::sort(order.begin(), order.end());
        std::vector<KeyValuePair> sorted;
        sorted.reserve(entries.size());
        for (const auto& item : order)
        {
            sorted.push_back(std::move(entries[item.second]));
        }
        entries = std::move(sorted);
    }

    // A value as pprint prints it: Markup as Markup('...')
    static std::string Print(const InternalValue& val, const RenderContext* context)
    {
        auto text = Apply<PrettyPrinter>(val, context);
        return val.IsMarkup() && IsStringValue(val) ? "Markup(" + text + ")" : text;
    }

    std::string operator()(const ListRef& list) const
    {
        CheckStack();
        std::string str;
        auto os = std::back_inserter(str);

        // A tuple prints as Python's: (a, b), and (a,) with one item
        const bool isTuple = list.IsTuple();
        str += isTuple ? "(" : "[";
        size_t count = 0;

        for (const auto& v : list)
        {
            if (count++ != 0)
            {
                fmt::format_to(os, ", ");
            }
            fmt::format_to(os, "{}", Print(v, m_context));
        }
        str += isTuple ? (count == 1 ? ",)" : ")") : "]";

        return str;
    }

    std::string operator()(const MapRef& map) const
    {
        CheckStack();
        std::string str;
        if (map.GetObjectClass())
        {
            // An object (namespace(), cycler(), joiner()) prints as its repr
            visitors::ValueRenderer<char>(str, true)(map);
            return str;
        }
        auto os = std::back_inserter(str);

        fmt::format_to(os, "{{");

        // Python's pprint sorts dict keys; UTF-8 byte order is code point order
        auto entries = map.GetEntries();
        SortEntriesByKey(entries);

        bool isFirst = true;
        for (auto& entry : entries)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                fmt::format_to(os, ", ");
            }

            fmt::format_to(os, "{}: ", Print(entry.key, m_context));
            fmt::format_to(os, "{}", Print(entry.value, m_context));
        }

        fmt::format_to(os, "}}");

        return str;
    }

    // A dictsort or items pair is a (key, value) tuple
    std::string operator()(const KeyValuePair& kwPair) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "({}, ", Print(kwPair.key, m_context));
        fmt::format_to(os, "{})", Print(kwPair.value, m_context));

        return str;
    }

    // Strings print as their repr: quoted the way Python picks, with escapes
    template<typename S>
    static std::string Repr(const S& str)
    {
        std::string result;
        visitors::ValueRenderer<char>(result, true)(str);
        return result;
    }

    std::string operator()(const std::string& str) const { return Repr(str); }

    std::string operator()(const std::string_view& str) const { return Repr(str); }

    std::string operator()(const std::wstring& str) const { return Repr(str); }

    std::string operator()(const std::wstring_view& str) const { return Repr(str); }

    std::string operator()(bool val) const { return val ? "True"s : "False"s; }

    std::string operator()(NoneValue) const { return "None"s; }
    std::string operator()(const UndefinedValue&) const { return "Undefined"s; }

    // A macro prints as its repr; Python's repr of a function names its address
    std::string operator()(const Callable& callable) const
    {
        std::string str;
        visitors::ValueRenderer<char>(str, true)(callable);
        return str.empty() ? "<callable>"s : str;
    }

    // Python's float repr: the shortest round-trip form, 1.0 rather than 1
    std::string operator()(double val) const { return Repr(val); }

    std::string operator()(int64_t val) const { return fmt::format("{}", val); }

    std::string operator()(const BigIntView& val) const { return std::string(CheckedDecimal(val)); }

    const RenderContext* m_context;
};

PrettyPrint::PrettyPrint(const FilterParams& params)
{
    ParseParams(NoArguments(), params);
}

InternalValue PrettyPrint::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return PrettyPrinter::Print(baseVal, &context);
}

Serialize::Serialize(const FilterParams& params, const Serialize::Mode mode)
    : m_mode(mode)
{
    switch (mode)
    {
    case JsonMode:
    {
        static const auto args = MakeArgumentsTable({ { "indent", false } });
        ParseParams(args, params);
        break;
    }
    default:
        ParseParams(NoArguments(), params);
        break;
    }
}

namespace
{

// The code point of one character as SplitCodePoints splits UTF-8: the lead byte keeps 5, 4 or
// 3 bits by its own value, so malformed text (an ASCII lead or a long run of continuation
// bytes) gives some value rather than an out-of-range shift
uint32_t Utf8CharValue(std::string_view ch)
{
    const auto lead = static_cast<unsigned char>(ch[0]);
    if (lead < 0x80 || ch.size() == 1)
    {
        return lead;
    }
    uint32_t cp = lead & (lead >= 0xF0 ? 0x07U : lead >= 0xE0 ? 0x0FU
                                                              : 0x1FU);
    for (size_t n = 1; n < ch.size(); ++n)
    {
        cp = (cp << 6) | (static_cast<unsigned char>(ch[n]) & 0x3FU);
    }
    return cp;
}

// Python's json.dumps(value, sort_keys=True, indent=indent) with the default ensure_ascii, as
// Jinja2's htmlsafe_json_dumps escapes it: <, >, & and ' become \u escapes. Outside strings
// those characters can only come from the indent, which is escaped once up front.
class PythonJsonWriter
{
public:
    PythonJsonWriter(RenderContext& context, std::optional<std::string> indent)
        : m_context(context)
        , m_indent(HtmlSafeIndent(std::move(indent)))
    {
    }

    InternalValue Write(const InternalValue& value)
    {
        WriteValue(value, 0);
        return m_out.Build();
    }

private:
    static std::optional<std::string> HtmlSafeIndent(std::optional<std::string> indent)
    {
        if (!indent)
        {
            return indent;
        }
        std::string result;
        for (auto ch : *indent)
        {
            switch (ch)
            {
            case '<':
                result += "\\u003c";
                break;
            case '>':
                result += "\\u003e";
                break;
            case '&':
                result += "\\u0026";
                break;
            case '\'':
                result += "\\u0027";
                break;
            default:
                result.push_back(ch);
                break;
            }
        }
        return result;
    }

    [[noreturn]] void Fail() const
    {
        m_context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
        throw std::runtime_error("tojson(): value is not JSON serializable");
    }

    void NewLine(size_t level)
    {
        if (!m_indent)
        {
            return;
        }
        m_out.push_back('\n');
        for (size_t n = 0; n != level; ++n)
        {
            m_out += *m_indent;
        }
    }

    // Bytes json.dumps keeps as they are and htmlsafe_json_dumps does not escape: printable
    // ASCII but ", \, <, >, & and '
    static bool IsPlainJsonByte(unsigned char ch)
    {
        return ch >= 0x20 && ch < 0x7F && ch != '"' && ch != '\\' && ch != '<' && ch != '>' && ch != '&' && ch != '\'';
    }

    void AppendUnit(uint32_t unit)
    {
        static const char hexDigits[] = "0123456789abcdef";
        m_out += "\\u";
        for (int shift = 12; shift >= 0; shift -= 4)
        {
            m_out.push_back(hexDigits[(unit >> shift) & 0xF]);
        }
    }

    // Writes runs of plain bytes with one append and decodes a code point only where a byte
    // needs escaping. Characters split as SplitCodePoints does, so malformed UTF-8 renders
    // as before: continuation bytes belong to the character before them.
    void WriteString(std::string_view str)
    {
        m_out.push_back('"');
        size_t pos = 0;
        while (pos != str.size())
        {
            auto start = pos;
            while (pos != str.size() && IsPlainJsonByte(static_cast<unsigned char>(str[pos])))
            {
                ++pos;
            }
            if (pos != str.size() && pos != start && IsCodePointTail(str[pos]))
            {
                --pos;
            }
            m_out.append(str.data() + start, pos - start);
            if (pos == str.size())
            {
                break;
            }
            auto end = pos + 1;
            while (end != str.size() && IsCodePointTail(str[end]))
            {
                ++end;
            }
            WriteChar(str.substr(pos, end - pos));
            pos = end;
        }
        m_out.push_back('"');
    }

    void WriteChar(std::string_view ch)
    {
        const auto cp = Utf8CharValue(ch);
        switch (cp)
        {
        case '"':
            m_out += "\\\"";
            break;
        case '\\':
            m_out += "\\\\";
            break;
        case '\n':
            m_out += "\\n";
            break;
        case '\r':
            m_out += "\\r";
            break;
        case '\t':
            m_out += "\\t";
            break;
        case '\b':
            m_out += "\\b";
            break;
        case '\f':
            m_out += "\\f";
            break;
        default:
            if (cp < 0x20 || cp >= 0x7F || cp == '<' || cp == '>' || cp == '&' || cp == '\'')
            {
                if (cp >= 0x10000)
                {
                    AppendUnit(0xD800 + ((cp - 0x10000) >> 10));
                    AppendUnit(0xDC00 + ((cp - 0x10000) & 0x3FF));
                }
                else
                {
                    AppendUnit(cp);
                }
            }
            else
            {
                m_out.push_back(static_cast<char>(cp));
            }
            break;
        }
    }

    template<typename Items, typename WriteItem>
    void WriteContainer(char open, char close, const Items& items, size_t level, const WriteItem& writeItem)
    {
        m_out.push_back(open);
        bool isFirst = true;
        for (auto& item : items)
        {
            // A generator with no end must not grow the text past the cap
            CheckSequenceSize(m_out.View().size(), "a JSON document");
            if (!isFirst)
            {
                m_out += m_indent ? "," : ", ";
            }
            isFirst = false;
            if (m_indent)
            {
                NewLine(level + 1);
            }
            writeItem(item);
        }
        if (m_indent && !isFirst)
        {
            NewLine(level);
        }
        m_out.push_back(close);
    }

    void WriteValue(const InternalValue& value, size_t level)
    {
        // Python's json.dumps raises RecursionError on values nested deeper than its stack
        CheckStack();
        if (const auto* info = GetUndefinedInfo(value))
        {
            // json.dumps knows no Undefined; a plain undefined (a host's null) stays null
            throw std::runtime_error(std::string("Object of type ") + UndefinedClassName(*info) + " is not JSON serializable");
        }
        if (value.IsNone() || value.IsUndefined())
        {
            m_out += "null";
        }
        else if (const auto* b = GetIf<bool>(&value))
        {
            m_out += *b ? "true" : "false";
        }
        else if (const auto* i = GetIf<int64_t>(&value))
        {
            m_out += std::to_string(*i);
        }
        else if (const auto big = AsBigInt(value))
        {
            m_out += CheckedDecimal(*big);
        }
        else if (const auto* d = GetIf<double>(&value))
        {
            m_out += JsonFloatText(*d);
        }
        else if (auto str = GetAsSameString(std::string(), value))
        {
            WriteString(*str);
        }
        else if (const auto* pair = GetIf<KeyValuePair>(&value))
        {
            InternalValueList items{ pair->key, pair->value };
            WriteContainer('[', ']', items, level, [this, level](const InternalValue& item) { WriteValue(item, level + 1); });
        }
        else if (auto list = AsList(value))
        {
            WriteContainer('[', ']', *list, level, [this, level](const InternalValue& item) { WriteValue(item, level + 1); });
        }
        else if (auto map = AsMap(value))
        {
            if (const char* objectClass = map->GetObjectClass())
            {
                // namespace(), cycler() and joiner() are objects, which json.dumps rejects
                throw std::runtime_error("Object of type "s + objectClass + " is not JSON serializable");
            }
            // sort_keys: Python orders str keys by code point, which is UTF-8 byte order
            auto entries = map->GetEntries();
            std::vector<std::pair<std::string, size_t>> order;
            order.reserve(entries.size());
            if (std::all_of(entries.begin(), entries.end(), [](const KeyValuePair& entry) { return IsStringValue(entry.key); }))
            {
                for (size_t idx = 0; idx != entries.size(); ++idx)
                {
                    order.emplace_back(AsString(entries[idx].key), idx);
                }
                std::sort(order.begin(), order.end());
            }
            else
            {
                OrderNonStringKeys(entries, order);
            }
            WriteContainer('{', '}', order, level, [this, &entries, level](const std::pair<std::string, size_t>& item) {
                WriteString(item.first);
                m_out += ": ";
                WriteValue(entries[item.second].value, level + 1);
            });
        }
        else
        {
            Fail();
        }
    }

    // How json.dumps writes a float, also as a key
    static std::string JsonFloatText(double val)
    {
        if (std::isnan(val))
        {
            return "NaN";
        }
        if (std::isinf(val))
        {
            return val < 0 ? "-Infinity" : "Infinity";
        }
        return std::string(visitors::PythonFloatText(val).View());
    }

    // Python's `a < b` for two keys that json.dumps accepts: numbers (bool included) order
    // among themselves, strings among themselves, and anything else is a TypeError
    static bool KeyLess(const InternalValue& left, const InternalValue& right)
    {
        const bool leftString = IsStringValue(left);
        const bool rightString = IsStringValue(right);
        if (leftString && rightString)
        {
            return AsString(left) < AsString(right);
        }
        if (leftString || rightString || left.IsNone() || right.IsNone())
        {
            throw std::runtime_error("'<' not supported between instances of '"s + Apply<visitors::PythonTypeNameGetter>(left) + "' and '" + Apply<visitors::PythonTypeNameGetter>(right) + "'");
        }
        return ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, BinaryExpression::LogicalLt, BinaryExpression::CaseSensitive));
    }

    // The keys of a mapping with a key that is not a string, in sort_keys order, with the JSON
    // spelling of each: 1 is "1", 2.5 is "2.5", True is "true" and None is "null". A tuple or
    // an object is no JSON key; keys of types Python cannot compare (a str and an int) are a TypeError
    static void OrderNonStringKeys(const std::vector<KeyValuePair>& entries, std::vector<std::pair<std::string, size_t>>& order)
    {
        for (size_t idx = 0; idx != entries.size(); ++idx)
        {
            const auto& key = entries[idx].key;
            std::string text;
            if (const auto* i = GetIf<int64_t>(&key))
            {
                text = std::to_string(*i);
            }
            else if (const auto big = AsBigInt(key))
            {
                text = CheckedDecimal(*big);
            }
            else if (const auto* b = GetIf<bool>(&key))
            {
                text = *b ? "true" : "false";
            }
            else if (const auto* d = GetIf<double>(&key))
            {
                text = JsonFloatText(*d);
            }
            else if (key.IsNone())
            {
                text = "null";
            }
            else if (IsStringValue(key))
            {
                text = AsString(key);
            }
            else
            {
                throw std::runtime_error("keys must be str, int, float, bool or None, not "s + Apply<visitors::PythonTypeNameGetter>(key));
            }
            order.emplace_back(std::move(text), idx);
        }
        std::stable_sort(order.begin(), order.end(), [&entries](const auto& left, const auto& right) { return KeyLess(entries[left.second].key, entries[right.second].key); });
    }

    RenderContext& m_context;
    std::optional<std::string> m_indent;
    StringBuilder<char> m_out;
};

// toyaml: a YAML document in block style that YAML 1.1 and 1.2 loaders read back as the same
// data. Mappings keep their order, nested collections indent by two spaces, empty ones are
// [] and {}, and the document has no trailing newline. A string is a plain scalar only when
// it cannot be read as anything else: it starts with a letter, _, / or a printable non-ASCII
// character, holds only those, digits, spaces and -.()+, ends with no space and is no
// null/bool word (y, n, yes, no, on, off, true, false, null in any case). Any other string is
// double-quoted with YAML escapes.
class YamlWriter
{
public:
    // Key bytes past which a key is written explicit: quoting expands a byte to at most four
    // (\xHH), so 255 bytes and the quotes stay within the 1024 characters of an implicit key
    static constexpr size_t MaxImplicitKey = 255;

    explicit YamlWriter(RenderContext& context)
        : m_context(context)
    {
    }

    InternalValue Write(const InternalValue& value)
    {
        WriteNode(value, 0, Position::Document);
        return m_out.Build();
    }

private:
    [[noreturn]] void Fail() const
    {
        m_context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
        throw std::runtime_error("toyaml(): value is not YAML serializable");
    }

    // The characters of a UTF-8 string as code points, each with its bytes; continuation bytes
    // belong to the character before them, as SplitCodePoints splits them
    template<typename Fn>
    static void ForEachChar(std::string_view str, const Fn& fn)
    {
        size_t pos = 0;
        while (pos != str.size())
        {
            auto end = pos + 1;
            while (end != str.size() && IsCodePointTail(str[end]))
            {
                ++end;
            }
            const auto ch = str.substr(pos, end - pos);
            fn(Utf8CharValue(ch), ch);
            pos = end;
        }
    }

    static bool IsAsciiLetter(uint32_t cp) { return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z'); }

    static bool IsPlain(std::string_view str)
    {
        if (str.empty() || str.back() == ' ')
        {
            return false;
        }
        static const std::string_view words[] = { "y", "n", "yes", "no", "on", "off", "true", "false", "null" };
        for (const auto word : words)
        {
            if (std::equal(str.begin(), str.end(), word.begin(), word.end(), [](char l, char r) { return (l | 0x20) == r; }))
            {
                return false;
            }
        }
        bool plain = true;
        bool first = true;
        ForEachChar(str, [&plain, &first](uint32_t cp, std::string_view) {
            const bool startChar = IsAsciiLetter(cp) || cp == '_' || cp == '/' || (cp >= 0x80 && detail::IsPythonPrintable(cp));
            const bool innerChar = (cp >= '0' && cp <= '9') || cp == ' ' || cp == '-' || cp == '.' || cp == '(' || cp == ')' || cp == '+';
            plain = plain && (startChar || (!first && innerChar));
            first = false;
        });
        return plain;
    }

    void WriteHex(char kind, uint32_t cp, int digits)
    {
        static const char hexDigits[] = "0123456789ABCDEF";
        m_out.push_back('\\');
        m_out.push_back(kind);
        for (int shift = (digits - 1) * 4; shift >= 0; shift -= 4)
        {
            m_out.push_back(hexDigits[(cp >> shift) & 0xF]);
        }
    }

    void WriteString(std::string_view str)
    {
        if (IsPlain(str))
        {
            m_out.append(str.data(), str.size());
            return;
        }
        m_out.push_back('"');
        ForEachChar(str, [this](uint32_t cp, std::string_view ch) {
            switch (cp)
            {
            case '"':
                m_out += "\\\"";
                return;
            case '\\':
                m_out += "\\\\";
                return;
            case '\0':
                m_out += "\\0";
                return;
            case '\a':
                m_out += "\\a";
                return;
            case '\b':
                m_out += "\\b";
                return;
            case '\t':
                m_out += "\\t";
                return;
            case '\n':
                m_out += "\\n";
                return;
            case '\v':
                m_out += "\\v";
                return;
            case '\f':
                m_out += "\\f";
                return;
            case '\r':
                m_out += "\\r";
                return;
            case 0x1B:
                m_out += "\\e";
                return;
            default:
                break;
            }
            if (cp < 0x20 || cp == 0x7F)
            {
                WriteHex('x', cp, 2);
            }
            else if (cp < 0x80 || detail::IsPythonPrintable(cp))
            {
                m_out.append(ch.data(), ch.size());
            }
            else if (cp < 0x10000)
            {
                WriteHex('u', cp, 4);
            }
            else
            {
                WriteHex('U', cp, 8);
            }
        });
        m_out.push_back('"');
    }

    // YAML 1.1 reads a float only with a dot in it: 1e+20 is written 1.0e+20
    void WriteFloat(double val)
    {
        if (std::isnan(val))
        {
            m_out += ".nan";
            return;
        }
        if (std::isinf(val))
        {
            m_out += val < 0 ? "-.inf" : ".inf";
            return;
        }
        const auto text = visitors::PythonFloatText(val);
        const auto view = text.View();
        const auto exp = view.find('e');
        if (exp != std::string_view::npos && view.find('.') == std::string_view::npos)
        {
            m_out += view.substr(0, exp);
            m_out += ".0";
            m_out += view.substr(exp);
            return;
        }
        m_out += view;
    }

    // Every item but a collection's first starts a line: the document stays within the cap
    // a generator with no end, or quadratic indentation, would pass
    void NewLine(size_t indent)
    {
        CheckSequenceSize(m_out.View().size(), "a YAML document");
        m_out.push_back('\n');
        for (size_t n = 0; n != indent; ++n)
        {
            m_out.push_back(' ');
        }
    }

    // Where a node starts: the document start, after a sequence item's "-" or after a
    // mapping key's ":"
    enum class Position
    {
        Document,
        SequenceItem,
        MappingValue
    };

    // Before a scalar, an empty collection or the first line of a collection: nothing at the
    // document start, a space after "-" (a nested collection starts on that line) and, after
    // ":", a space or a new line at indent
    void StartNode(Position pos, size_t indent, bool blockCollection)
    {
        if (pos == Position::MappingValue && blockCollection)
        {
            NewLine(indent);
        }
        else if (pos != Position::Document)
        {
            m_out.push_back(' ');
        }
    }

    // Writes value with its lines at column indent
    void WriteNode(const InternalValue& value, size_t indent, Position pos)
    {
        CheckStack();
        if (const auto* info = GetUndefinedInfo(value))
        {
            throw std::runtime_error(std::string("Object of type ") + UndefinedClassName(*info) + " is not YAML serializable");
        }
        auto scalar = [this, indent, pos] { StartNode(pos, indent, false); };
        if (value.IsNone() || value.IsUndefined())
        {
            scalar();
            m_out += "null";
        }
        else if (const auto* b = GetIf<bool>(&value))
        {
            scalar();
            m_out += *b ? "true" : "false";
        }
        else if (const auto* i = GetIf<int64_t>(&value))
        {
            scalar();
            m_out += std::to_string(*i);
        }
        else if (const auto big = AsBigInt(value))
        {
            scalar();
            m_out += CheckedDecimal(*big);
        }
        else if (const auto* d = GetIf<double>(&value))
        {
            scalar();
            WriteFloat(*d);
        }
        else if (auto str = GetAsSameString(std::string(), value))
        {
            scalar();
            WriteString(*str);
        }
        else if (const auto* pair = GetIf<KeyValuePair>(&value))
        {
            const InternalValueList items{ InternalValue(pair->key), pair->value };
            WriteSequence(items, indent, pos);
        }
        else if (auto list = AsList(value))
        {
            WriteSequence(*list, indent, pos);
        }
        else if (auto map = AsMap(value))
        {
            WriteMapping(*map, indent, pos);
        }
        else
        {
            Fail();
        }
    }

    template<typename Items>
    void WriteSequence(const Items& items, size_t indent, Position pos)
    {
        bool isFirst = true;
        for (const auto& item : items)
        {
            if (isFirst)
            {
                StartNode(pos, indent, true);
            }
            else
            {
                NewLine(indent);
            }
            isFirst = false;
            m_out.push_back('-');
            WriteNode(item, indent + 2, Position::SequenceItem);
        }
        if (isFirst)
        {
            StartNode(pos, indent, false);
            m_out += "[]";
        }
    }

    void WriteMapping(const MapRef& map, size_t indent, Position pos)
    {
        if (const char* objectClass = map.GetObjectClass())
        {
            // namespace(), cycler() and joiner() are objects, not data
            throw std::runtime_error("Object of type "s + objectClass + " is not YAML serializable");
        }
        // A key that is not a string is written as its str() spelling (DictKeyText), as at the
        // public boundary (docs/tasks/0036)
        const auto entries = map.GetEntries();
        std::vector<std::string> keys;
        keys.reserve(entries.size());
        for (const auto& entry : entries)
        {
            keys.push_back(IsStringValue(entry.key) ? AsString(entry.key) : DictKeyText(entry.key));
        }
        if (keys.empty())
        {
            StartNode(pos, indent, false);
            m_out += "{}";
            return;
        }
        bool isFirst = true;
        for (size_t idx = 0; idx != keys.size(); ++idx)
        {
            const auto& key = keys[idx];
            if (isFirst)
            {
                StartNode(pos, indent, true);
            }
            else
            {
                NewLine(indent);
            }
            isFirst = false;
            if (key.size() > MaxImplicitKey)
            {
                // YAML limits an implicit key to 1024 characters; a longer one is "? key" with
                // the value on a ": " line of its own
                m_out += "? ";
                WriteString(key);
                NewLine(indent);
            }
            else
            {
                WriteString(key);
            }
            m_out.push_back(':');
            WriteNode(entries[idx].value, indent + 2, Position::MappingValue);
        }
    }

    RenderContext& m_context;
    StringBuilder<char> m_out;
};

} // namespace

InternalValue Serialize::Filter(const InternalValue& value, RenderContext& context)
{
    if (m_mode == YamlMode)
    {
        // YAML is no HTML: under autoescape the result is escaped like any string
        return YamlWriter(context).Write(value);
    }

    // Jinja2's do_tojson: json.dumps with sort_keys=True, then htmlsafe_json_dumps escapes
    // <, >, & and ' so the result is safe in HTML and <script> (PythonJsonWriter does both)
    auto indentVal = this->GetArgumentValue("indent", context);
    // json.dumps encodes a str before it looks at indent, so only other values check it
    if (!IsEmpty(indentVal) && (IsStringValue(value) || GetAsSameString(std::string(), value)))
    {
        indentVal = InternalValue();
    }
    std::optional<std::string> indent;
    if (auto str = GetAsSameString(std::string(), indentVal))
    {
        indent = *str;
    }
    else if (const auto* count = GetIf<int64_t>(&indentVal))
    {
        const auto spaces = std::max<int64_t>(0, *count);
        CheckSequenceSize(static_cast<uint64_t>(spaces), "an indentation");
        indent = std::string(static_cast<size_t>(spaces), ' ');
    }
    else if (const auto* flag = GetIf<bool>(&indentVal))
    {
        indent = std::string(*flag ? 1 : 0, ' ');
    }
    else if (const auto* undefinedInfo = GetUndefinedInfo(indentVal))
    {
        ThrowUndefined(*undefinedInfo);
    }
    else if (!IsEmpty(indentVal))
    {
        throw std::runtime_error("can't multiply sequence by non-int of type '"s + Apply<visitors::PythonTypeNameGetter>(indentVal) + "'");
    }

    InternalValue resultVal = PythonJsonWriter(context, std::move(indent)).Write(value);
    // tojson output is always Markup
    resultVal.SetMarkup();
    return resultVal;
}

void StringFormat::SetConstantBase(const InternalValue& base)
{
    auto format = AsStringView<char>(base);
    if (format && !base.IsMarkup())
    {
        m_constFormat = std::make_shared<const CompiledPercentFormat>(std::string(*format));
    }
}

InternalValue StringFormat::Filter(const InternalValue& baseVal, RenderContext& context)
{
    // Jinja2's do_format is printf-style: str(value) % (kwargs or args)
    auto* callback = context.GetRendererCallback();
    // A narrow string is used in place; anything else is converted to its text first
    std::string ownedFormat;
    std::string_view format;
    // A literal format was parsed at Load
    if (!m_constFormat)
    {
        auto formatView = AsStringView<char>(baseVal);
        if (!formatView)
        {
            ownedFormat = AsString(InternalValue(callback->GetAsTargetString(baseVal)));
            formatView = ownedFormat;
        }
        format = *formatView;
    }
    // str() of an undefined is empty, and Python's '' % args has an argument left over
    if (GetUndefinedInfo(baseVal) && !m_params.posParams.empty())
    {
        throw std::runtime_error("not all arguments converted during string formatting");
    }
    if (!m_params.posParams.empty() && !m_params.kwParams.empty())
    {
        throw std::runtime_error("format(): can't handle positional and keyword arguments at the same time");
    }
    auto params = helpers::EvaluateCallParams(m_params, context);
    bool markup = baseVal.IsMarkup();
    std::string formatted;
    if (!params.kwParams.empty())
    {
        InternalDict mapping;
        for (auto& [name, value] : params.kwParams)
        {
            mapping[name] = value;
        }
        InternalValue values = CreateMapAdapter(std::move(mapping));
        // Markup % args escapes what the arguments convert to and stays Markup
        formatted = m_constFormat ? m_constFormat->Format(values) : PythonPercentFormat(format, values, markup);
    }
    else
    {
        formatted = m_constFormat ? m_constFormat->Format(params.posParams.data(), params.posParams.size())
                                  : PythonPercentFormat(format, params.posParams.data(), params.posParams.size(), markup);
    }
    InternalValue result(formatted);
    result.SetMarkup(baseVal.IsMarkup());
    return result;
}

XmlAttrFilter::XmlAttrFilter(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "autospace", false, true } });
    ParseParams(args, params);
}

InternalValue XmlAttrFilter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    auto map = AsMap(baseVal);
    // An object such as namespace() has no items()
    if (!map || map->GetObjectClass())
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }

    auto escape = [](const std::string& str) { return EscapeHtml(std::string_view(str)); };

    // Port of Jinja2's do_xmlattr: key="escaped str(value)" in the mapping's order, None and
    // undefined values left out (and callables, which have no str() here)
    std::string result;
    for (auto& [keyValue, value] : map->GetEntries())
    {
        if (IsEmpty(value) || GetIf<Callable>(&value))
        {
            continue;
        }
        // Jinja2 searches the key for whitespace with a regular expression, which only takes text
        if (!IsStringValue(keyValue))
        {
            throw std::runtime_error("expected string or bytes-like object, got '"s + Apply<visitors::PythonTypeNameGetter>(keyValue) + "'");
        }
        const auto key = AsString(keyValue);
        // Jinja2 rejects keys with whitespace, "/", ">" or "="
        if (std::any_of(key.begin(), key.end(), [](char ch) { return std::strchr(" \t\n\r\f\v/>=", ch) != nullptr && ch != 0; }))
        {
            throw std::runtime_error("xmlattr(): invalid character in attribute name: '" + key + "'");
        }
        auto text = AsString(InternalValue(context.GetRendererCallback()->GetAsTargetString(value)));
        result += (result.empty() ? "" : " ") + escape(key) + "=\"" + (value.IsMarkup() ? text : escape(text)) + "\"";
    }

    if (!result.empty() && ConvertToBool(GetArgumentValue("autospace", context)))
    {
        result.insert(0, 1, ' ');
    }
    // Markup under autoescape, a plain str otherwise
    InternalValue resultVal(std::move(result));
    resultVal.SetMarkup(context.IsAutoescape());
    return resultVal;
}

} // namespace filters
} // namespace v2
} // namespace jinja2
