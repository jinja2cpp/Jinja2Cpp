#include "json_reader.h"

#include "internal_value.h"
#include "make_unexpected.h"

#include <jinja2cpp/value.h>

#include <charconv>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace jinja2
{
inline namespace v2
{
namespace
{
// A recursive-descent reader. Recursion is bounded by JsonReadLimits::maxDepth, and every
// failure records its message and offset and unwinds through `false`.
class JsonReader
{
public:
    JsonReader(std::string_view text, const JsonReadLimits& limits)
        : m_text(text)
        , m_maxDepth(limits.maxDepth)
        , m_maxItems(limits.maxSequenceSize == 0 || limits.maxSequenceSize > MaxSequenceSize ? MaxSequenceSize : static_cast<std::size_t>(limits.maxSequenceSize))
    {
    }

    // The document's one value; with objectOnly, which must be an object
    nonstd::expected<Value, std::string> ReadDocument(bool objectOnly)
    {
        if (m_text.substr(0, 3) == "\xEF\xBB\xBF")
        {
            return MakeUnexpected(Message("Unexpected UTF-8 BOM (decode using utf-8-sig)", 0));
        }
        SkipWhitespace();
        if (objectOnly && (m_pos == m_text.size() || m_text[m_pos] != '{'))
        {
            return MakeUnexpected(Message(m_pos == m_text.size() || !StartsValue(m_text[m_pos]) ? "Expecting value" : "Expecting an object", m_pos));
        }
        Value result;
        if (!ReadValue(result, 0))
        {
            return MakeUnexpected(Message(m_error, m_errorPos));
        }
        SkipWhitespace();
        if (m_pos != m_text.size())
        {
            return MakeUnexpected(Message("Extra data", m_pos));
        }
        return result;
    }

private:
    static bool StartsValue(char ch)
    {
        return ch == '[' || ch == '"' || ch == '-' || (ch >= '0' && ch <= '9') || ch == 't' || ch == 'f' || ch == 'n';
    }

    bool Fail(const char* message, std::size_t pos)
    {
        m_error = message;
        m_errorPos = pos;
        return false;
    }

    // Python's format: "<message>: line 1 column 5 (char 4)", column and offset counted in code points
    [[nodiscard]] std::string Message(std::string_view message, std::size_t pos) const
    {
        std::size_t line = 1;
        std::size_t chars = 0;
        std::size_t lineStartChars = 0;
        for (std::size_t i = 0; i != pos; ++i)
        {
            const auto byte = static_cast<unsigned char>(m_text[i]);
            if ((byte & 0xC0U) != 0x80U)
            {
                ++chars;
            }
            if (byte == '\n')
            {
                ++line;
                lineStartChars = chars;
            }
        }
        std::string result(message);
        result += ": line ";
        result += std::to_string(line);
        result += " column ";
        result += std::to_string(chars - lineStartChars + 1);
        result += " (char ";
        result += std::to_string(chars);
        result += ')';
        return result;
    }

    void SkipWhitespace()
    {
        while (m_pos != m_text.size())
        {
            const char ch = m_text[m_pos];
            if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r')
            {
                break;
            }
            ++m_pos;
        }
    }

    bool ReadValue(Value& value, std::size_t depth)
    {
        if (m_pos == m_text.size())
        {
            return Fail("Expecting value", m_pos);
        }
        switch (m_text[m_pos])
        {
        case '{':
        {
            ValuesMap map;
            if (!ReadObject(map, depth))
            {
                return false;
            }
            value = Value(std::move(map));
            return true;
        }
        case '[':
        {
            ValuesList list;
            if (!ReadArray(list, depth))
            {
                return false;
            }
            value = Value(std::move(list));
            return true;
        }
        case '"':
        {
            std::string text;
            if (!ReadString(text))
            {
                return false;
            }
            value = Value(std::move(text));
            return true;
        }
        case 't':
            return ReadLiteral("true", Value(true), value);
        case 'f':
            return ReadLiteral("false", Value(false), value);
        case 'n':
            return ReadLiteral("null", Value(), value);
        default:
            return ReadNumber(value);
        }
    }

    bool ReadLiteral(std::string_view word, Value literal, Value& value)
    {
        if (m_text.substr(m_pos, word.size()) != word)
        {
            return Fail("Expecting value", m_pos);
        }
        m_pos += word.size();
        value = std::move(literal);
        return true;
    }

    bool Enter(std::size_t depth)
    {
        if (depth >= m_maxDepth)
        {
            return Fail("Nesting too deep", m_pos);
        }
        ++m_pos;
        SkipWhitespace();
        return true;
    }

    bool ReadObject(ValuesMap& map, std::size_t depth)
    {
        if (!Enter(depth))
        {
            return false;
        }
        if (m_pos != m_text.size() && m_text[m_pos] == '}')
        {
            ++m_pos;
            return true;
        }
        while (true)
        {
            if (m_pos == m_text.size() || m_text[m_pos] != '"')
            {
                return Fail("Expecting property name enclosed in double quotes", m_pos);
            }
            std::string key;
            if (!ReadString(key))
            {
                return false;
            }
            SkipWhitespace();
            if (m_pos == m_text.size() || m_text[m_pos] != ':')
            {
                return Fail("Expecting ':' delimiter", m_pos);
            }
            ++m_pos;
            SkipWhitespace();
            Value item;
            if (!ReadValue(item, depth + 1))
            {
                return false;
            }
            map.insert_or_assign(std::move(key), std::move(item));
            if (map.size() > m_maxItems)
            {
                return Fail("Too many items", m_pos);
            }
            SkipWhitespace();
            if (m_pos != m_text.size() && m_text[m_pos] == '}')
            {
                ++m_pos;
                return true;
            }
            if (m_pos == m_text.size() || m_text[m_pos] != ',')
            {
                return Fail("Expecting ',' delimiter", m_pos);
            }
            ++m_pos;
            SkipWhitespace();
        }
    }

    bool ReadArray(ValuesList& list, std::size_t depth)
    {
        if (!Enter(depth))
        {
            return false;
        }
        if (m_pos != m_text.size() && m_text[m_pos] == ']')
        {
            ++m_pos;
            return true;
        }
        while (true)
        {
            if (list.size() == m_maxItems)
            {
                return Fail("Too many items", m_pos);
            }
            Value item;
            if (!ReadValue(item, depth + 1))
            {
                return false;
            }
            list.push_back(std::move(item));
            SkipWhitespace();
            if (m_pos != m_text.size() && m_text[m_pos] == ']')
            {
                ++m_pos;
                return true;
            }
            if (m_pos == m_text.size() || m_text[m_pos] != ',')
            {
                return Fail("Expecting ',' delimiter", m_pos);
            }
            ++m_pos;
            SkipWhitespace();
        }
    }

    static int HexDigit(char ch)
    {
        if (ch >= '0' && ch <= '9')
        {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f')
        {
            return ch - 'a' + 10;
        }
        if (ch >= 'A' && ch <= 'F')
        {
            return ch - 'A' + 10;
        }
        return -1;
    }

    // The four hex digits after a `\u` at m_pos, or -1
    int ReadHex4()
    {
        if (m_text.size() - m_pos < 6)
        {
            return -1;
        }
        int code = 0;
        for (std::size_t i = 2; i != 6; ++i)
        {
            const int digit = HexDigit(m_text[m_pos + i]);
            if (digit < 0)
            {
                return -1;
            }
            code = (code * 16) + digit;
        }
        return code;
    }

    static void AppendUtf8(std::string& out, std::uint32_t code)
    {
        if (code < 0x80)
        {
            out += static_cast<char>(code);
        }
        else if (code < 0x800)
        {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
        else if (code < 0x10000)
        {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
        else
        {
            out += static_cast<char>(0xF0 | (code >> 18));
            out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    // The length of the well-formed UTF-8 sequence at m_pos (RFC 3629: no overlong forms,
    // no surrogates, nothing past U+10FFFF), or 0
    [[nodiscard]] std::size_t Utf8Length() const
    {
        const auto byte = [this](std::size_t i) { return m_pos + i < m_text.size() ? static_cast<unsigned char>(m_text[m_pos + i]) : 0U; };
        const unsigned lead = byte(0);
        std::size_t length = 0;
        // The range of the second byte, narrower than 0x80-0xBF after E0, ED, F0 and F4
        unsigned low = 0x80;
        unsigned high = 0xBF;
        if (lead >= 0xC2 && lead <= 0xDF)
        {
            length = 2;
        }
        else if (lead >= 0xE0 && lead <= 0xEF)
        {
            length = 3;
            low = lead == 0xE0 ? 0xA0 : low;
            high = lead == 0xED ? 0x9F : high;
        }
        else if (lead >= 0xF0 && lead <= 0xF4)
        {
            length = 4;
            low = lead == 0xF0 ? 0x90 : low;
            high = lead == 0xF4 ? 0x8F : high;
        }
        else
        {
            return 0;
        }
        if (byte(1) < low || byte(1) > high)
        {
            return 0;
        }
        for (std::size_t i = 2; i != length; ++i)
        {
            if ((byte(i) & 0xC0U) != 0x80U)
            {
                return 0;
            }
        }
        return length;
    }

    bool ReadString(std::string& out)
    {
        const std::size_t start = m_pos;
        ++m_pos;
        while (true)
        {
            if (m_pos == m_text.size())
            {
                return Fail("Unterminated string starting at", start);
            }
            if (out.size() > m_maxItems)
            {
                return Fail("String too long", start);
            }
            const char ch = m_text[m_pos];
            if (ch == '"')
            {
                ++m_pos;
                return true;
            }
            if (static_cast<unsigned char>(ch) < 0x20)
            {
                return Fail("Invalid control character at", m_pos);
            }
            if (static_cast<unsigned char>(ch) >= 0x80)
            {
                const std::size_t length = Utf8Length();
                if (length == 0)
                {
                    return Fail("Invalid UTF-8 at", m_pos);
                }
                out.append(m_text.substr(m_pos, length));
                m_pos += length;
                continue;
            }
            if (ch != '\\')
            {
                out += ch;
                ++m_pos;
                continue;
            }
            if (m_pos + 1 == m_text.size())
            {
                return Fail("Unterminated string starting at", start);
            }
            switch (m_text[m_pos + 1])
            {
            case '"':
                out += '"';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case 'b':
                out += '\b';
                break;
            case 'f':
                out += '\f';
                break;
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case 'u':
                if (!ReadUnicodeEscape(out))
                {
                    return false;
                }
                continue;
            default:
                return Fail("Invalid \\escape", m_pos);
            }
            m_pos += 2;
        }
    }

    // A `\uXXXX` escape at m_pos, with the low half of a surrogate pair after it
    bool ReadUnicodeEscape(std::string& out)
    {
        const int code = ReadHex4();
        if (code < 0)
        {
            return Fail("Invalid \\uXXXX escape", m_pos + 1);
        }
        const std::size_t escape = m_pos;
        m_pos += 6;
        if (code >= 0xDC00 && code <= 0xDFFF)
        {
            return Fail("Unpaired surrogate", escape);
        }
        if (code < 0xD800 || code > 0xDBFF)
        {
            AppendUtf8(out, static_cast<std::uint32_t>(code));
            return true;
        }
        if (m_text.substr(m_pos, 2) != "\\u")
        {
            return Fail("Unpaired surrogate", escape);
        }
        const int low = ReadHex4();
        if (low < 0)
        {
            return Fail("Invalid \\uXXXX escape", m_pos + 1);
        }
        if (low < 0xDC00 || low > 0xDFFF)
        {
            return Fail("Unpaired surrogate", escape);
        }
        m_pos += 6;
        AppendUtf8(out, 0x10000U + ((static_cast<std::uint32_t>(code) - 0xD800U) << 10U) + (static_cast<std::uint32_t>(low) - 0xDC00U));
        return true;
    }

    [[nodiscard]] bool IsDigit(std::size_t pos) const { return pos < m_text.size() && m_text[pos] >= '0' && m_text[pos] <= '9'; }

    [[nodiscard]] std::size_t SkipDigits(std::size_t pos) const
    {
        while (IsDigit(pos))
        {
            ++pos;
        }
        return pos;
    }

    // The end of the fraction and exponent after the integer part ending at `pos`. A '.' or
    // exponent without digits ends the number before it, as in Python's json, so that what
    // follows is reported
    [[nodiscard]] std::size_t SkipFractionAndExponent(std::size_t pos) const
    {
        if (pos < m_text.size() && m_text[pos] == '.' && IsDigit(pos + 1))
        {
            pos = SkipDigits(pos + 1);
        }
        if (pos < m_text.size() && (m_text[pos] == 'e' || m_text[pos] == 'E'))
        {
            std::size_t digits = pos + 1;
            if (digits < m_text.size() && (m_text[digits] == '+' || m_text[digits] == '-'))
            {
                ++digits;
            }
            if (IsDigit(digits))
            {
                pos = SkipDigits(digits);
            }
        }
        return pos;
    }

    bool ReadNumber(Value& value)
    {
        const std::size_t start = m_pos;
        std::size_t pos = m_pos;
        if (pos < m_text.size() && m_text[pos] == '-')
        {
            ++pos;
        }
        if (!IsDigit(pos))
        {
            return Fail("Expecting value", start);
        }
        pos = m_text[pos] == '0' ? pos + 1 : SkipDigits(pos);
        const std::size_t integerEnd = pos;
        pos = SkipFractionAndExponent(pos);
        const std::string_view number = m_text.substr(start, pos - start);
        if (number.size() > m_maxItems)
        {
            // ToDouble copies the digits, so a number is bounded as a string is
            return Fail("Number too long", start);
        }
        m_pos = pos;
        if (pos == integerEnd)
        {
            std::int64_t result = 0;
            const auto [end, ec] = std::from_chars(number.data(), number.data() + number.size(), result);
            if (ec == std::errc() && end == number.data() + number.size())
            {
                value = Value(result);
                return true;
            }
        }
        value = Value(ToDouble(number));
        return true;
    }

    // strtod reads the decimal point of the C locale in force; the number is spelled with it
    static double ToDouble(std::string_view number)
    {
        std::string text(number);
        const std::string_view point = std::localeconv()->decimal_point;
        if (point != ".")
        {
            const auto dot = text.find('.');
            if (dot != std::string::npos)
            {
                text.replace(dot, 1, point);
            }
        }
        // An exponent out of range gives HUGE_VAL (inf) or 0, as Python's float() does
        return std::strtod(text.c_str(), nullptr);
    }

    std::string_view m_text;
    std::size_t m_pos = 0;
    std::size_t m_maxDepth;
    std::size_t m_maxItems;
    std::string m_error;
    std::size_t m_errorPos = 0;
};
} // namespace

nonstd::expected<ValuesMap, std::string> ReadJsonObject(std::string_view text, const JsonReadLimits& limits)
{
    auto result = JsonReader(text, limits).ReadDocument(true);
    if (!result)
    {
        return MakeUnexpected(std::move(result.error()));
    }
    return std::move(result.value().AsMap());
}

nonstd::expected<Value, std::string> ReadJsonValue(std::string_view text, const JsonReadLimits& limits)
{
    return JsonReader(text, limits).ReadDocument(false);
}

} // namespace v2
} // namespace jinja2
