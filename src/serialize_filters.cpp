#include "expression_evaluator.h"
#include "filters.h"
#include "internal_value.h"
#include "markup.h"
#include "out_stream.h" // IWYU pragma: keep (GetStreamOnString returns an OutStream by value)
#include "python_format.h"
#include "render_context.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <fmt/args.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

using namespace std::string_literals;

namespace jinja2::filters
{
struct PrettyPrinter : visitors::BaseVisitor<std::string>
{
    using BaseVisitor::operator();

    explicit PrettyPrinter(const RenderContext* context)
        : m_context(context)
    {
    }

    std::string operator()(const ListAdapter& list) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "[");
        bool isFirst = true;

        for (const auto& v : list)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                fmt::format_to(os, ", ");
            }
            fmt::format_to(os, "{}", Apply<PrettyPrinter>(v, m_context));
        }
        fmt::format_to(os, "]");

        return str;
    }

    std::string operator()(const MapAdapter& map) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "{{");

        // Python's pprint sorts dict keys; UTF-8 byte order is code point order
        auto keys = map.GetKeys();
        std::sort(keys.begin(), keys.end());

        bool isFirst = true;
        for (auto& k : keys)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                fmt::format_to(os, ", ");
            }

            fmt::format_to(os, "'{}': ", k);
            fmt::format_to(os, "{}", Apply<PrettyPrinter>(map.GetValueByName(k), m_context));
        }

        fmt::format_to(os, "}}");

        return str;
    }

    std::string operator()(const KeyValuePair& kwPair) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "'{}': ", kwPair.key);
        fmt::format_to(os, "{}", Apply<PrettyPrinter>(kwPair.value, m_context));

        return str;
    }

    std::string operator()(const std::string& str) const { return fmt::format("'{}'", str); }

    std::string operator()(const std::string_view& str) const { return fmt::format("'{}'", fmt::basic_string_view<char>(str.data(), str.size())); }

    std::string operator()(const std::wstring& str) const { return fmt::format("'{}'", ConvertString<std::string>(str)); }

    std::string operator()(const std::wstring_view& str) const { return fmt::format("'{}'", ConvertString<std::string>(str)); }

    std::string operator()(bool val) const { return val ? "true"s : "false"s; }

    std::string operator()(EmptyValue) const { return "none"s; }
    std::string operator()(const UndefinedValue&) const { return "none"s; }

    std::string operator()(const Callable&) const { return "<callable>"s; }

    std::string operator()(double val) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "{:.8g}", val);

        return str;
    }

    std::string operator()(int64_t val) const { return fmt::format("{}", val); }

    const RenderContext* m_context;
};

PrettyPrint::PrettyPrint(const FilterParams& params)
{
    ParseParams({}, params);
}

InternalValue PrettyPrint::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return Apply<PrettyPrinter>(baseVal, &context);
}

Serialize::Serialize(const FilterParams& params, const Serialize::Mode mode)
    : m_mode(mode)
{
    switch (mode)
    {
    case JsonMode:
        ParseParams({ { "indent", false } }, params);
        break;
    default:
        break;
    }
}

namespace
{

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

    std::string Write(const InternalValue& value)
    {
        WriteValue(value, 0);
        return std::move(m_out);
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
        // The lead byte keeps 7, 5, 4 or 3 bits for 1 to 4 byte sequences
        uint32_t cp = static_cast<unsigned char>(ch[0]);
        if (ch.size() > 1)
        {
            cp &= 0x7FU >> ch.size();
        }
        for (size_t n = 1; n < ch.size(); ++n)
        {
            cp = (cp << 6) | (static_cast<unsigned char>(ch[n]) & 0x3F);
        }
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
        if (++m_depth > 1000)
        {
            Fail();
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
        else if (const auto* d = GetIf<double>(&value))
        {
            if (std::isnan(*d))
            {
                m_out += "NaN";
            }
            else if (std::isinf(*d))
            {
                m_out += *d < 0 ? "-Infinity" : "Infinity";
            }
            else
            {
                m_out += visitors::FormatPythonFloat(*d);
            }
        }
        else if (auto str = GetAsSameString(std::string(), value))
        {
            WriteString(*str);
        }
        else if (const auto* pair = GetIf<KeyValuePair>(&value))
        {
            InternalValueList items{ InternalValue(pair->key), pair->value };
            WriteContainer('[', ']', items, level, [this, level](const InternalValue& item) { WriteValue(item, level + 1); });
        }
        else if (const auto* list = GetIf<ListAdapter>(&value))
        {
            WriteContainer('[', ']', *list, level, [this, level](const InternalValue& item) { WriteValue(item, level + 1); });
        }
        else if (const auto* map = GetIf<MapAdapter>(&value))
        {
            // sort_keys: Python orders str keys by code point, which is UTF-8 byte order
            auto keys = map->GetKeys();
            std::sort(keys.begin(), keys.end());
            WriteContainer('{', '}', keys, level, [this, map, level](const std::string& key) {
                WriteString(key);
                m_out += ": ";
                WriteValue(map->GetValueByName(key), level + 1);
            });
        }
        else
        {
            Fail();
        }
        --m_depth;
    }

    RenderContext& m_context;
    std::optional<std::string> m_indent;
    std::string m_out;
    size_t m_depth = 0;
};

} // namespace

InternalValue Serialize::Filter(const InternalValue& value, RenderContext& context)
{
    if (m_mode != JsonMode)
    {
        return InternalValue();
    }

    // Jinja2's do_tojson: json.dumps with sort_keys=True, then htmlsafe_json_dumps escapes
    // <, >, & and ' so the result is safe in HTML and <script> (PythonJsonWriter does both)
    auto indentVal = this->GetArgumentValue("indent", context);
    std::optional<std::string> indent;
    if (auto str = GetAsSameString(std::string(), indentVal))
    {
        indent = *str;
    }
    else if (!IsEmpty(indentVal))
    {
        indent = std::string(static_cast<size_t>(std::max<int64_t>(0, ConvertToInt(indentVal))), ' ');
    }

    auto result = PythonJsonWriter(context, std::move(indent)).Write(value);
    // tojson output is always Markup
    InternalValue resultVal(std::move(result));
    resultVal.SetMarkup();
    return resultVal;
}

namespace
{

using FormatContext = fmt::format_context;
using FormatArgument = fmt::basic_format_arg<FormatContext>;
using FormatDynamicArgsStore = fmt::dynamic_format_arg_store<FormatContext>;

struct FormatArgumentConverter : visitors::BaseVisitor<FormatArgument>
{
    using result_t = FormatArgument;

    using BaseVisitor::operator();

    FormatArgumentConverter(const RenderContext* context, FormatDynamicArgsStore& store)
        : m_context(context)
        , m_store(store)
    {
    }

    FormatArgumentConverter(const RenderContext* context, FormatDynamicArgsStore& store, std::string name)
        : m_context(context)
        , m_store(store)
        , m_name(std::move(name))
        , m_named(true)
    {
    }

    result_t operator()(const ListAdapter& list) const { return MakeResult(Apply<PrettyPrinter>(list, m_context)); }

    result_t operator()(const MapAdapter& map) const { return MakeResult(Apply<PrettyPrinter>(map, m_context)); }

    result_t operator()(const std::string& str) const { return MakeResult(str); }

    result_t operator()(const std::string_view& str) const { return MakeResult(std::string(str.data(), str.size())); }

    result_t operator()(const std::wstring& str) const { return MakeResult(ConvertString<std::string>(str)); }

    result_t operator()(const std::wstring_view& str) const { return MakeResult(ConvertString<std::string>(str)); }

    result_t operator()(double val) const { return MakeResult(val); }

    result_t operator()(int64_t val) const { return MakeResult(val); }

    result_t operator()(bool val) const { return MakeResult(val ? "true"s : "false"s); }

    result_t operator()(EmptyValue) const { return MakeResult("none"s); }
    result_t operator()(const UndefinedValue&) const { return MakeResult("none"s); }

    result_t operator()(const Callable&) const { return MakeResult("<callable>"s); }

    template<typename T>
    [[nodiscard]] result_t MakeResult(const T& t) const
    {
        if (!m_named)
        {
            m_store.push_back(t);
        }
        else
        {
            m_store.push_back(fmt::arg(m_name.c_str(), t));
        }
        return fmt::basic_format_arg<FormatContext>(t);
    }

    const RenderContext* m_context{};
    FormatDynamicArgsStore& m_store;
    const std::string m_name;
    bool m_named = false;
};

} // namespace

InternalValue StringFormat::Filter(const InternalValue& baseVal, RenderContext& context)
{
    // Jinja2's do_format is printf-style: str(value) % (kwargs or args). A format string
    // without "%" keeps Jinja2C++'s own {}-style formatting (fmt syntax)
    auto* callback = context.GetRendererCallback();
    auto format = AsString(InternalValue(callback->GetAsTargetString(baseVal)));
    if (format.find('%') != std::string::npos)
    {
        if (!m_params.posParams.empty() && !m_params.kwParams.empty())
        {
            throw std::runtime_error("format(): can't handle positional and keyword arguments at the same time");
        }
        auto params = helpers::EvaluateCallParams(m_params, context);
        InternalValue values;
        if (!params.kwParams.empty())
        {
            InternalValueMap mapping;
            for (auto& [name, value] : params.kwParams)
            {
                mapping[name] = value;
            }
            values = CreateMapAdapter(std::move(mapping));
        }
        else
        {
            values = ListAdapter::CreateAdapter(std::move(params.posParams)).MarkAsTuple();
        }
        // Markup % args escapes the arguments and stays Markup
        if (baseVal.IsMarkup())
        {
            values = EscapeFormatArgs(values, callback);
        }
        InternalValue result(PythonPercentFormat(format, values));
        result.SetMarkup(baseVal.IsMarkup());
        return result;
    }

    // Format library internally likes using non-owning views to complex arguments.
    // In order to ensure proper lifetime of values and named args,
    // helper buffer is created and passed to visitors.
    FormatDynamicArgsStore store;
    auto evalArg = [&](auto& expr) {
        auto val = expr->Evaluate(context);
        return baseVal.IsMarkup() ? EscapeFormatArg(val, callback) : val;
    };
    for (auto& arg : m_params.posParams)
    {
        Apply<FormatArgumentConverter>(evalArg(arg), &context, store);
    }

    for (auto& [name, expr] : m_params.kwParams)
    {
        Apply<FormatArgumentConverter>(evalArg(expr), &context, store, name);
    }

    InternalValue result(fmt::vformat(format, store));
    result.SetMarkup(baseVal.IsMarkup());
    return result;
}

XmlAttrFilter::XmlAttrFilter(const FilterParams& params)
{
    ParseParams({ { "autospace", false, true } }, params);
}

InternalValue XmlAttrFilter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const auto* map = GetIf<MapAdapter>(&baseVal);
    if (!map)
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }

    auto escape = [](const std::string& str) { return EscapeHtml(std::string_view(str)); };

    // Port of Jinja2's do_xmlattr: key="escaped str(value)" in the mapping's order, None and
    // undefined values left out (and callables, which have no str() here)
    std::string result;
    for (auto& key : map->GetKeys())
    {
        auto value = map->GetValueByName(key);
        if (IsEmpty(value) || GetIf<Callable>(&value))
        {
            continue;
        }
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

} // namespace jinja2::filters
