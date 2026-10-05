#include "python_format.h"

#include "internal_value.h"
#include "value_visitors.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace jinja2
{
namespace
{

std::string Str(const InternalValue& val, bool asRepr = false)
{
    std::string result;
    Apply<visitors::ValueRenderer<char>>(val, result, asRepr);
    return result;
}

const char* TypeName(const InternalValue& val)
{
    if (GetIf<double>(&val))
    {
        return "float";
    }
    if (GetIf<int64_t>(&val))
    {
        return "int";
    }
    if (GetIf<bool>(&val))
    {
        return "bool";
    }
    if (val.IsNone())
    {
        return "NoneType";
    }
    if (GetIf<MapAdapter>(&val))
    {
        return "dict";
    }
    if (const auto* list = GetIf<ListAdapter>(&val))
    {
        return list->IsTuple() ? "tuple" : "list";
    }
    return "str";
}

size_t CodePoints(std::string_view str)
{
    return static_cast<size_t>(std::count_if(str.begin(), str.end(), [](char ch) { return (static_cast<unsigned char>(ch) & 0xC0) != 0x80; }));
}

void AppendUtf8(std::string& out, uint32_t cp)
{
    if (cp < 0x80)
    {
        out.push_back(static_cast<char>(cp));
    }
    else if (cp < 0x800)
    {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp < 0x10000)
    {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else
    {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

struct Spec
{
    bool left = false;
    bool plus = false;
    bool space = false;
    bool zero = false;
    bool alternate = false;
    int64_t width = -1;
    int64_t precision = -1;
};

std::string_view Sign(bool negative, const Spec& spec)
{
    if (negative)
    {
        return "-";
    }
    if (spec.plus)
    {
        return "+";
    }
    return spec.space ? " " : "";
}

// Appends a converted value padded to the width: zeros go after the sign and the 0x/0o prefix
void AppendPadded(std::string& out, std::string_view body, const Spec& spec, bool isNumber)
{
    auto length = static_cast<int64_t>(CodePoints(body));
    if (spec.width <= length)
    {
        out.append(body);
        return;
    }
    auto fill = static_cast<size_t>(spec.width - length);
    if (spec.left)
    {
        out.append(body);
        out.append(fill, ' ');
        return;
    }
    if (!spec.zero || !isNumber)
    {
        out.append(fill, ' ');
        out.append(body);
        return;
    }
    size_t pos = 0;
    if (pos < body.size() && (body[pos] == '-' || body[pos] == '+' || body[pos] == ' '))
    {
        ++pos;
    }
    if (pos + 1 < body.size() && body[pos] == '0' && std::strchr("xXob", body[pos + 1]))
    {
        pos += 2;
    }
    out.append(body.substr(0, pos));
    out.append(fill, '0');
    out.append(body.substr(pos));
}

void AppendInteger(std::string& out, int64_t value, char conversion, const Spec& spec)
{
    auto magnitude = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    // 22 octal digits hold any 64-bit magnitude
    char digitsBuffer[24];
    char* digitsEnd = digitsBuffer;
    std::string_view prefix;
    switch (conversion)
    {
    case 'x':
        digitsEnd = fmt::format_to(digitsEnd, "{:x}", magnitude);
        prefix = spec.alternate ? "0x" : "";
        break;
    case 'X':
        digitsEnd = fmt::format_to(digitsEnd, "{:X}", magnitude);
        prefix = spec.alternate ? "0X" : "";
        break;
    case 'o':
        digitsEnd = fmt::format_to(digitsEnd, "{:o}", magnitude);
        prefix = spec.alternate ? "0o" : "";
        break;
    default:
        break;
    }
    // Decimal, the common case, skips fmt's format string
    const fmt::format_int decimal(magnitude);
    std::string_view digits = digitsEnd != digitsBuffer ? std::string_view(digitsBuffer, static_cast<size_t>(digitsEnd - digitsBuffer))
                                                        : std::string_view(decimal.data(), decimal.size());
    auto sign = Sign(value < 0, spec);
    // Every piece is ASCII, so the byte length is the width it takes
    size_t precisionZeros = spec.precision > static_cast<int64_t>(digits.size()) ? static_cast<size_t>(spec.precision) - digits.size() : 0;
    auto length = static_cast<int64_t>(sign.size() + prefix.size() + precisionZeros + digits.size());
    auto fill = spec.width > length ? static_cast<size_t>(spec.width - length) : 0;
    if (fill != 0 && !spec.left && !spec.zero)
    {
        out.append(fill, ' ');
    }
    out.append(sign);
    out.append(prefix);
    out.append(precisionZeros + (spec.zero && !spec.left ? fill : 0), '0');
    out.append(digits);
    if (fill != 0 && spec.left)
    {
        out.append(fill, ' ');
    }
}

void AppendFloat(std::string& out, double value, char conversion, const Spec& spec)
{
    auto precision = spec.precision < 0 ? 6 : spec.precision;
    // The fmt spec mirrors the flags: "{:+#.{}e}" at most
    char format[16];
    size_t size = 0;
    format[size++] = '{';
    format[size++] = ':';
    if (spec.plus)
    {
        format[size++] = '+';
    }
    else if (spec.space)
    {
        format[size++] = ' ';
    }
    if (spec.alternate)
    {
        format[size++] = '#';
    }
    for (char ch : std::string_view(".{}"))
    {
        format[size++] = ch;
    }
    format[size++] = conversion;
    format[size++] = '}';
    // Short results stay in the buffer's inline storage
    fmt::memory_buffer body;
    fmt::format_to(std::back_inserter(body), fmt::runtime(std::string_view(format, size)), value, precision);
    AppendPadded(out, std::string_view(body.data(), body.size()), spec, true);
}

class Formatter
{
public:
    Formatter(std::string_view format, const InternalValue& values)
        : m_format(format)
        , m_map(GetIf<MapAdapter>(&values))
    {
        const auto* list = GetIf<ListAdapter>(&values);
        if (list && list->IsTuple())
        {
            // A tuple is read item by item; a list of unknown size is copied once
            if (auto size = list->GetSize())
            {
                m_list = list;
                m_count = *size;
            }
            else
            {
                m_owned = list->ToValueList();
                m_args = m_owned.data();
                m_count = m_owned.size();
            }
        }
        else
        {
            m_args = &values;
            m_count = 1;
        }
        // A mapping is used by key, so it is never "not all arguments converted"
        m_isMapping = m_map != nullptr;
    }

    Formatter(std::string_view format, const InternalValue* args, size_t count)
        : m_format(format)
        , m_args(args)
        , m_count(count)
    {
    }

    std::string Run()
    {
        std::string result;
        result.reserve(m_format.size() + 16);
        for (m_pos = 0; m_pos < m_format.size();)
        {
            auto percent = m_format.find('%', m_pos);
            result.append(m_format.substr(m_pos, percent == std::string_view::npos ? std::string_view::npos : percent - m_pos));
            if (percent == std::string_view::npos)
            {
                break;
            }
            m_pos = percent + 1;
            Directive(result);
        }
        if (!m_isMapping && m_next < m_count)
        {
            throw std::runtime_error("not all arguments converted during string formatting");
        }
        return result;
    }

private:
    [[nodiscard]] char Peek() const { return m_pos < m_format.size() ? m_format[m_pos] : '\0'; }

    void Incomplete() const
    {
        if (m_pos >= m_format.size())
        {
            throw std::runtime_error("incomplete format");
        }
    }

    // The reference stays valid until the next call
    const InternalValue& NextArg()
    {
        if (m_keyed)
        {
            return m_keyedValue;
        }
        if (m_next >= m_count)
        {
            throw std::runtime_error("not enough arguments for format string");
        }
        auto index = m_next++;
        if (m_list)
        {
            m_listItem = m_list->GetValueByIndex(static_cast<int64_t>(index));
            return m_listItem;
        }
        return m_args[index];
    }

    int64_t StarArg()
    {
        const auto& val = NextArg();
        // Clamped like a literal width, so a huge or INT64_MIN star neither allocates nor overflows on negation
        if (const auto* i = GetIf<int64_t>(&val))
        {
            return std::max<int64_t>(-(1 << 20), std::min<int64_t>(*i, 1 << 20));
        }
        if (const auto* b = GetIf<bool>(&val))
        {
            return *b ? 1 : 0;
        }
        throw std::runtime_error("* wants int");
    }

    int64_t Number()
    {
        int64_t result = 0;
        while (Peek() >= '0' && Peek() <= '9')
        {
            result = std::min<int64_t>((result * 10) + (m_format[m_pos++] - '0'), 1 << 20);
        }
        return result;
    }

    // Reads an optional "(key)" and selects that mapping value as the argument
    void Key()
    {
        m_keyed = false;
        if (Peek() != '(')
        {
            return;
        }
        if (!m_map)
        {
            throw std::runtime_error("format requires a mapping");
        }
        int depth = 1;
        auto keyStart = ++m_pos;
        for (; m_pos < m_format.size() && depth != 0; ++m_pos)
        {
            if (m_format[m_pos] == '(')
            {
                ++depth;
            }
            else if (m_format[m_pos] == ')')
            {
                --depth;
            }
        }
        if (depth != 0)
        {
            throw std::runtime_error("incomplete format key");
        }
        std::string key(m_format.substr(keyStart, m_pos - 1 - keyStart));
        if (!m_map->HasValue(key))
        {
            throw std::runtime_error("KeyError: '" + key + "'");
        }
        m_keyedValue = m_map->GetValueByName(key);
        m_keyed = true;
    }

    // Reads flags, width, precision and a length modifier
    Spec ParseSpec()
    {
        Spec spec;
        for (;; ++m_pos)
        {
            auto ch = Peek();
            if (ch == '-')
            {
                spec.left = true;
            }
            else if (ch == '+')
            {
                spec.plus = true;
            }
            else if (ch == ' ')
            {
                spec.space = true;
            }
            else if (ch == '#')
            {
                spec.alternate = true;
            }
            else if (ch == '0')
            {
                spec.zero = true;
            }
            else
            {
                break;
            }
        }
        if (Peek() == '*')
        {
            ++m_pos;
            spec.width = StarArg();
            if (spec.width < 0)
            {
                spec.left = true;
                spec.width = -spec.width;
            }
        }
        else if (Peek() >= '0' && Peek() <= '9')
        {
            spec.width = Number();
        }
        if (Peek() == '.')
        {
            ++m_pos;
            if (Peek() == '*')
            {
                ++m_pos;
                spec.precision = std::max<int64_t>(0, StarArg());
            }
            else
            {
                spec.precision = Number();
            }
        }
        // One length modifier is accepted and ignored
        if (Peek() == 'h' || Peek() == 'l' || Peek() == 'L')
        {
            ++m_pos;
        }
        return spec;
    }

    void Directive(std::string& out)
    {
        auto start = m_pos;
        Incomplete();
        Key();
        auto spec = ParseSpec();
        Incomplete();

        auto conversion = m_format[m_pos++];
        // Only a bare "%%" is a literal percent; Python rejects '%' after a key, flags or width
        if (conversion == '%' && m_pos - 1 == start)
        {
            out.push_back('%');
            return;
        }

        const auto& arg = NextArg();
        switch (conversion)
        {
        case 's':
        case 'r':
        case 'a':
            ConvertText(out, arg, conversion, spec);
            break;
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':
            ConvertInteger(out, arg, conversion, spec);
            break;
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
            ConvertFloat(out, arg, conversion, spec);
            break;
        case 'c':
            ConvertChar(out, arg, spec);
            break;
        default:
            throw std::runtime_error(fmt::format("unsupported format character '{}' (0x{:x}) at index {}", conversion, static_cast<unsigned char>(conversion), m_pos - 1));
        }
    }

    static void ConvertText(std::string& out, const InternalValue& arg, char conversion, const Spec& spec)
    {
        // str() of a narrow string is the string itself, so it is not copied
        std::string owned;
        auto view = conversion == 's' ? NarrowStringView(arg) : std::nullopt;
        if (!view)
        {
            owned = Str(arg, conversion != 's');
            view = owned;
        }
        auto text = *view;
        if (spec.precision >= 0 && static_cast<size_t>(spec.precision) < CodePoints(text))
        {
            size_t count = 0;
            size_t cut = 0;
            for (; cut < text.size(); ++cut)
            {
                if ((static_cast<unsigned char>(text[cut]) & 0xC0) != 0x80 && count++ == static_cast<size_t>(spec.precision))
                {
                    break;
                }
            }
            text = text.substr(0, cut);
        }
        AppendPadded(out, text, spec, false);
    }

    static void ConvertInteger(std::string& out, const InternalValue& arg, char conversion, const Spec& spec)
    {
        bool isDecimal = conversion == 'd' || conversion == 'i' || conversion == 'u';
        if (const auto* i = GetIf<int64_t>(&arg))
        {
            AppendInteger(out, *i, conversion, spec);
            return;
        }
        if (const auto* b = GetIf<bool>(&arg))
        {
            AppendInteger(out, *b ? 1 : 0, conversion, spec);
            return;
        }
        const auto* d = GetIf<double>(&arg);
        if (d && isDecimal)
        {
            if (std::isnan(*d))
            {
                throw std::runtime_error("cannot convert float NaN to integer");
            }
            if (std::isinf(*d) || std::fabs(*d) >= 9223372036854775808.0)
            {
                throw std::runtime_error("cannot convert float infinity to integer");
            }
            AppendInteger(out, static_cast<int64_t>(*d), conversion, spec);
            return;
        }
        throw std::runtime_error(fmt::format("%{} format: {} is required, not {}", conversion, isDecimal ? "a real number" : "an integer", TypeName(arg)));
    }

    static void ConvertFloat(std::string& out, const InternalValue& arg, char conversion, const Spec& spec)
    {
        // Python prints NaN without its sign bit
        if (const auto* d = GetIf<double>(&arg))
        {
            AppendFloat(out, std::isnan(*d) ? std::fabs(*d) : *d, conversion, spec);
            return;
        }
        if (const auto* i = GetIf<int64_t>(&arg))
        {
            AppendFloat(out, static_cast<double>(*i), conversion, spec);
            return;
        }
        if (const auto* b = GetIf<bool>(&arg))
        {
            AppendFloat(out, *b ? 1.0 : 0.0, conversion, spec);
            return;
        }
        throw std::runtime_error(fmt::format("must be real number, not {}", TypeName(arg)));
    }

    static void ConvertChar(std::string& out, const InternalValue& arg, const Spec& spec)
    {
        std::string text;
        int64_t cp = -1;
        if (const auto* i = GetIf<int64_t>(&arg))
        {
            cp = *i;
        }
        else if (const auto* b = GetIf<bool>(&arg))
        {
            cp = *b ? 1 : 0;
        }
        else if (!GetIf<double>(&arg) && !IsEmpty(arg))
        {
            auto str = GetAsSameString(std::string(), arg);
            if (!str || CodePoints(*str) != 1)
            {
                throw std::runtime_error("%c requires int or char");
            }
            text = *str;
        }
        else
        {
            throw std::runtime_error("%c requires int or char");
        }
        if (text.empty())
        {
            if (cp < 0 || cp > 0x10FFFF)
            {
                throw std::runtime_error("%c arg not in range(0x110000)");
            }
            AppendUtf8(text, static_cast<uint32_t>(cp));
        }
        AppendPadded(out, text, spec, false);
    }

    std::string_view m_format;
    size_t m_pos = 0;
    const MapAdapter* m_map = nullptr;
    bool m_isMapping = false;
    // Positional arguments: an array, or a tuple read by index
    const InternalValue* m_args = nullptr;
    const ListAdapter* m_list = nullptr;
    size_t m_count = 0;
    InternalValueList m_owned;
    InternalValue m_listItem;
    size_t m_next = 0;
    bool m_keyed = false;
    InternalValue m_keyedValue;
};

} // namespace

std::string PythonPercentFormat(std::string_view format, const InternalValue& values)
{
    return Formatter(format, values).Run();
}

std::string PythonPercentFormat(std::string_view format, const InternalValue* args, size_t count)
{
    return Formatter(format, args, count).Run();
}

std::optional<std::string_view> NarrowStringView(const InternalValue& val)
{
    if (const auto* str = GetIf<std::string>(&val))
    {
        return *str;
    }
    if (const auto* target = GetIf<TargetString>(&val))
    {
        if (const auto* str = std::get_if<std::string>(target))
        {
            return *str;
        }
    }
    if (const auto* target = GetIf<TargetStringView>(&val))
    {
        if (const auto* str = std::get_if<std::string_view>(target))
        {
            return *str;
        }
    }
    return std::nullopt;
}

} // namespace jinja2
