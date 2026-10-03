#include "python_format.h"

#include "internal_value.h"
#include "value_visitors.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

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

size_t CodePoints(const std::string& str)
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

std::string Sign(bool negative, const Spec& spec)
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

// Pads a converted value to the width: zeros go after the sign and the 0x/0o prefix
std::string Pad(std::string body, const Spec& spec, bool isNumber)
{
    auto length = static_cast<int64_t>(CodePoints(body));
    if (spec.width <= length)
    {
        return body;
    }
    auto fill = static_cast<size_t>(spec.width - length);
    if (spec.left)
    {
        return body + std::string(fill, ' ');
    }
    if (!spec.zero || !isNumber)
    {
        return std::string(fill, ' ') + body;
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
    body.insert(pos, fill, '0');
    return body;
}

std::string FormatInteger(int64_t value, char conversion, const Spec& spec)
{
    auto magnitude = value < 0 ? 0 - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
    std::string digits;
    std::string prefix;
    switch (conversion)
    {
    case 'x':
        digits = fmt::format("{:x}", magnitude);
        prefix = spec.alternate ? "0x" : "";
        break;
    case 'X':
        digits = fmt::format("{:X}", magnitude);
        prefix = spec.alternate ? "0X" : "";
        break;
    case 'o':
        digits = fmt::format("{:o}", magnitude);
        prefix = spec.alternate ? "0o" : "";
        break;
    default:
        digits = fmt::format("{}", magnitude);
        break;
    }
    if (spec.precision > static_cast<int64_t>(digits.size()))
    {
        digits.insert(0, static_cast<size_t>(spec.precision) - digits.size(), '0');
    }
    return Pad(Sign(value < 0, spec) + prefix + digits, spec, true);
}

std::string FormatFloat(double value, char conversion, const Spec& spec)
{
    auto precision = spec.precision < 0 ? 6 : spec.precision;
    std::string format = "{:";
    if (spec.plus)
    {
        format += '+';
    }
    else if (spec.space)
    {
        format += ' ';
    }
    if (spec.alternate)
    {
        format += '#';
    }
    format += ".{}";
    format += conversion;
    format += '}';
    auto body = fmt::format(fmt::runtime(format), value, precision);
    return Pad(body, spec, true);
}

class Formatter
{
public:
    Formatter(const std::string& format, const InternalValue& values)
        : m_format(format)
        , m_map(GetIf<MapAdapter>(&values))
    {
        const auto* list = GetIf<ListAdapter>(&values);
        if (list && list->IsTuple())
        {
            m_args = list->ToValueList();
        }
        else
        {
            m_args.push_back(values);
        }
        // A mapping is used by key, so it is never "not all arguments converted"
        m_isMapping = m_map != nullptr;
    }

    std::string Run()
    {
        std::string result;
        for (m_pos = 0; m_pos < m_format.size();)
        {
            auto percent = m_format.find('%', m_pos);
            result.append(m_format, m_pos, percent == std::string::npos ? std::string::npos : percent - m_pos);
            if (percent == std::string::npos)
            {
                break;
            }
            m_pos = percent + 1;
            result += Directive();
        }
        if (!m_isMapping && m_next < m_args.size())
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

    const InternalValue& NextArg()
    {
        if (m_keyed)
        {
            return m_keyedValue;
        }
        if (m_next >= m_args.size())
        {
            throw std::runtime_error("not enough arguments for format string");
        }
        return m_args[m_next++];
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
        auto key = m_format.substr(keyStart, m_pos - 1 - keyStart);
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

    std::string Directive()
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
            return "%";
        }

        const auto& arg = NextArg();
        switch (conversion)
        {
        case 's':
        case 'r':
        case 'a':
            return ConvertText(arg, conversion, spec);
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':
            return ConvertInteger(arg, conversion, spec);
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
            return ConvertFloat(arg, conversion, spec);
        case 'c':
            return ConvertChar(arg, spec);
        default:
            throw std::runtime_error(fmt::format("unsupported format character '{}' (0x{:x}) at index {}", conversion, static_cast<unsigned char>(conversion), m_pos - 1));
        }
    }

    static std::string ConvertText(const InternalValue& arg, char conversion, const Spec& spec)
    {
        auto text = Str(arg, conversion != 's');
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
            text.erase(cut);
        }
        return Pad(text, spec, false);
    }

    static std::string ConvertInteger(const InternalValue& arg, char conversion, const Spec& spec)
    {
        bool isDecimal = conversion == 'd' || conversion == 'i' || conversion == 'u';
        if (const auto* i = GetIf<int64_t>(&arg))
        {
            return FormatInteger(*i, conversion, spec);
        }
        if (const auto* b = GetIf<bool>(&arg))
        {
            return FormatInteger(*b ? 1 : 0, conversion, spec);
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
            return FormatInteger(static_cast<int64_t>(*d), conversion, spec);
        }
        throw std::runtime_error(fmt::format("%{} format: {} is required, not {}", conversion, isDecimal ? "a real number" : "an integer", TypeName(arg)));
    }

    static std::string ConvertFloat(const InternalValue& arg, char conversion, const Spec& spec)
    {
        // Python prints NaN without its sign bit
        if (const auto* d = GetIf<double>(&arg))
        {
            return FormatFloat(std::isnan(*d) ? std::fabs(*d) : *d, conversion, spec);
        }
        if (const auto* i = GetIf<int64_t>(&arg))
        {
            return FormatFloat(static_cast<double>(*i), conversion, spec);
        }
        if (const auto* b = GetIf<bool>(&arg))
        {
            return FormatFloat(*b ? 1.0 : 0.0, conversion, spec);
        }
        throw std::runtime_error(fmt::format("must be real number, not {}", TypeName(arg)));
    }

    static std::string ConvertChar(const InternalValue& arg, const Spec& spec)
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
        return Pad(text, spec, false);
    }

    const std::string& m_format;
    size_t m_pos = 0;
    const MapAdapter* m_map = nullptr;
    bool m_isMapping = false;
    InternalValueList m_args;
    size_t m_next = 0;
    bool m_keyed = false;
    InternalValue m_keyedValue;
};

} // namespace

std::string PythonPercentFormat(const std::string& format, const InternalValue& values)
{
    return Formatter(format, values).Run();
}

} // namespace jinja2
