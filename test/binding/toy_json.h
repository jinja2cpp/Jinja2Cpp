#ifndef JINJA2CPP_TEST_BINDING_TOY_JSON_H
#define JINJA2CPP_TEST_BINDING_TOY_JSON_H

// A dependency-free JSON DOM with two sets of jinja2::JsonTraits, to test the adapter contract without a library
// (docs/tasks/0311):
//  - ToyViewDoc: traits like a Qt-style library's. The Node is a custom handle (not std::reference_wrapper), AsString
//    and the member keys are std::string, and the traits do not derive from DomJsonTraits. Runs the whole suite.
//  - ToyStreamDoc: forward-only traits like simdjson on-demand's. Every function returns jinja2::JsonResult, there is
//    no Size, At or Find, Location reports a byte offset, an error can be injected, and integers beyond uint64 are
//    JsonKind::BigInt. Only jinja2::ToValue works.
// Neither is a model of a good JSON library: a repeated key stays twice in the object, as in simdjson's DOM.

#include <jinja2cpp/json_adapter.h>

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace toyjson
{
struct Value;
struct Member;

struct BigIntText
{
    std::string digits;
};

struct Value
{
    std::variant<std::monostate, bool, std::int64_t, std::uint64_t, double, std::string, BigIntText, std::vector<Value>, std::vector<Member>> data;
    std::uint64_t offset = 0; // byte offset of the value in the text
};

struct Member
{
    std::string key;
    Value value;
};

using Array = std::vector<Value>;
using Object = std::vector<Member>;

// A recursive-descent parser of the JSON in the suite. Integers beyond uint64 are doubles, or JsonKind::BigInt text
// when `bigIntText` is set. Throws std::runtime_error on malformed text.
class Parser
{
public:
    Parser(std::string_view text, bool bigIntText)
        : m_text(text)
        , m_bigIntText(bigIntText)
    {
    }

    Value Parse()
    {
        Value result = ParseValue();
        SkipSpace();
        if (m_pos != m_text.size())
        {
            Fail("trailing characters");
        }
        return result;
    }

private:
    [[noreturn]] void Fail(const char* what) const { throw std::runtime_error(std::string("toy JSON: ") + what + " at " + std::to_string(m_pos)); }

    static bool IsNumberChar(char ch) { return (ch >= '0' && ch <= '9') || ch == '-' || ch == '+'; }

    void SkipSpace()
    {
        while (m_pos < m_text.size() && (m_text[m_pos] == ' ' || m_text[m_pos] == '\n' || m_text[m_pos] == '\t' || m_text[m_pos] == '\r'))
        {
            ++m_pos;
        }
    }

    char Peek()
    {
        SkipSpace();
        if (m_pos >= m_text.size())
        {
            Fail("unexpected end");
        }
        return m_text[m_pos];
    }

    void Expect(char ch)
    {
        if (Peek() != ch)
        {
            Fail("unexpected character");
        }
        ++m_pos;
    }

    bool Consume(std::string_view word)
    {
        if (m_text.substr(m_pos, word.size()) != word)
        {
            return false;
        }
        m_pos += word.size();
        return true;
    }

    Value ParseValue()
    {
        const char ch = Peek();
        Value result;
        result.offset = m_pos;
        if (ch == '{')
        {
            result.data = ParseObject();
        }
        else if (ch == '[')
        {
            result.data = ParseArray();
        }
        else if (ch == '"')
        {
            result.data = ParseString();
        }
        else if (Consume("true"))
        {
            result.data = true;
        }
        else if (Consume("false"))
        {
            result.data = false;
        }
        else if (Consume("null"))
        {
            result.data = std::monostate{};
        }
        else
        {
            ParseNumber(result);
        }
        return result;
    }

    Object ParseObject()
    {
        Object members;
        Expect('{');
        if (Peek() == '}')
        {
            ++m_pos;
            return members;
        }
        for (;;)
        {
            if (Peek() != '"')
            {
                Fail("object key expected");
            }
            Member member;
            member.key = ParseString();
            Expect(':');
            member.value = ParseValue();
            members.push_back(std::move(member));
            if (Peek() == ',')
            {
                ++m_pos;
                continue;
            }
            Expect('}');
            return members;
        }
    }

    Array ParseArray()
    {
        Array items;
        Expect('[');
        if (Peek() == ']')
        {
            ++m_pos;
            return items;
        }
        for (;;)
        {
            items.push_back(ParseValue());
            if (Peek() == ',')
            {
                ++m_pos;
                continue;
            }
            Expect(']');
            return items;
        }
    }

    unsigned ParseHex4()
    {
        if (m_pos + 4 > m_text.size())
        {
            Fail("short \\u escape");
        }
        unsigned code = 0;
        for (int n = 0; n < 4; ++n)
        {
            const char ch = m_text[m_pos++];
            code <<= 4;
            if (ch >= '0' && ch <= '9')
            {
                code |= static_cast<unsigned>(ch - '0');
            }
            else if (ch >= 'a' && ch <= 'f')
            {
                code |= static_cast<unsigned>(ch - 'a' + 10);
            }
            else if (ch >= 'A' && ch <= 'F')
            {
                code |= static_cast<unsigned>(ch - 'A' + 10);
            }
            else
            {
                Fail("bad \\u escape");
            }
        }
        return code;
    }

    static void AppendUtf8(std::string& out, unsigned code)
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

    std::string ParseString()
    {
        Expect('"');
        std::string out;
        for (;;)
        {
            if (m_pos >= m_text.size())
            {
                Fail("unterminated string");
            }
            const char ch = m_text[m_pos++];
            if (ch == '"')
            {
                return out;
            }
            if (ch != '\\')
            {
                out += ch;
                continue;
            }
            if (m_pos >= m_text.size())
            {
                Fail("unterminated escape");
            }
            const char esc = m_text[m_pos++];
            switch (esc)
            {
            case '"':
            case '\\':
            case '/':
                out += esc;
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
            {
                unsigned code = ParseHex4();
                if (code >= 0xD800 && code < 0xDC00)
                {
                    if (!Consume("\\u"))
                    {
                        Fail("lone high surrogate");
                    }
                    const unsigned low = ParseHex4();
                    if (low < 0xDC00 || low > 0xDFFF)
                    {
                        Fail("bad low surrogate");
                    }
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                }
                AppendUtf8(out, code);
                break;
            }
            default:
                Fail("bad escape");
            }
        }
    }

    void ParseNumber(Value& result)
    {
        const std::size_t begin = m_pos;
        bool integral = true;
        while (m_pos < m_text.size())
        {
            const char ch = m_text[m_pos];
            if (ch == '.' || ch == 'e' || ch == 'E')
            {
                integral = false;
            }
            else if (!IsNumberChar(ch))
            {
                break;
            }
            ++m_pos;
        }
        if (begin == m_pos)
        {
            Fail("value expected");
        }
        const std::string_view token = m_text.substr(begin, m_pos - begin);
        if (integral)
        {
            std::int64_t asInt = 0;
            if (std::from_chars(token.data(), token.data() + token.size(), asInt).ec == std::errc())
            {
                result.data = asInt;
                return;
            }
            std::uint64_t asUInt = 0;
            if (token.front() != '-' && std::from_chars(token.data(), token.data() + token.size(), asUInt).ec == std::errc())
            {
                result.data = asUInt;
                return;
            }
            if (m_bigIntText)
            {
                result.data = BigIntText{ std::string(token) };
                return;
            }
        }
        result.data = std::strtod(std::string(token).c_str(), nullptr);
    }

    std::string_view m_text;
    bool m_bigIntText;
    std::size_t m_pos = 0;
};

inline jinja2::JsonKind KindOf(const Value& val)
{
    switch (val.data.index())
    {
    case 1:
        return jinja2::JsonKind::Bool;
    case 2:
        return jinja2::JsonKind::Int;
    case 3:
        return jinja2::JsonKind::UInt;
    case 4:
        return jinja2::JsonKind::Double;
    case 5:
        return jinja2::JsonKind::String;
    case 6:
        return jinja2::JsonKind::BigInt;
    case 7:
        return jinja2::JsonKind::Array;
    case 8:
        return jinja2::JsonKind::Object;
    default:
        return jinja2::JsonKind::Null;
    }
}
} // namespace toyjson

//! The document of the view traits
struct ToyViewDoc
{
    explicit ToyViewDoc(toyjson::Value val)
        : root(std::move(val))
    {
    }

    toyjson::Value root;
};

//! The document of the forward-only traits; the test can make a walk fail
struct ToyStreamDoc
{
    explicit ToyStreamDoc(toyjson::Value val)
        : root(std::move(val))
    {
    }

    toyjson::Value root;
    //! Fails the Kind call that is number `failAtValue` (0 is the root; values are visited in document order)
    std::int64_t failAtValue = -1;
    //! A string equal to this fails AsString
    std::string failOnString;
    //! ForEachMember fails after its first member
    bool failInObject = false;
    std::int64_t visited = 0;
};

namespace jinja2
{
inline namespace v2
{
//! Traits as a Qt-style library would give them: a handle node, std::string results, no DomJsonTraits base
template<>
struct JsonTraits<ToyViewDoc>
{
    struct Node
    {
        const toyjson::Value* value;
        bool operator==(const Node& other) const { return value == other.value; }
    };

    static Node Root(const ToyViewDoc& doc) { return Node{ &doc.root }; }
    static JsonKind Kind(Node node) { return toyjson::KindOf(*node.value); }
    static bool AsBool(Node node) { return std::get<bool>(node.value->data); }
    static std::int64_t AsInt(Node node) { return std::get<std::int64_t>(node.value->data); }
    static std::uint64_t AsUInt(Node node) { return std::get<std::uint64_t>(node.value->data); }
    static double AsDouble(Node node) { return std::get<double>(node.value->data); }
    static std::string AsString(Node node) { return std::get<std::string>(node.value->data); }

    static std::size_t Size(Node node)
    {
        if (const auto* items = std::get_if<toyjson::Array>(&node.value->data))
        {
            return items->size();
        }
        return std::get<toyjson::Object>(node.value->data).size();
    }
    static Node At(Node node, std::size_t idx) { return Node{ &std::get<toyjson::Array>(node.value->data)[idx] }; }
    // The first of the repeated keys
    static std::optional<Node> Find(Node node, std::string_view name)
    {
        for (const auto& member : std::get<toyjson::Object>(node.value->data))
        {
            if (member.key == name)
            {
                return Node{ &member.value };
            }
        }
        return std::nullopt;
    }
    template<typename F>
    static void ForEachMember(Node node, F onMember)
    {
        for (const auto& member : std::get<toyjson::Object>(node.value->data))
        {
            if (!onMember(std::string(member.key), Node{ &member.value }))
            {
                break;
            }
        }
    }
};

//! Forward-only traits: every function can fail, the byte offset of a value is known, nothing can be indexed
template<>
struct JsonTraits<ToyStreamDoc>
{
    struct Node
    {
        const toyjson::Value* value;
        ToyStreamDoc* doc;
    };

    static Node Root(ToyStreamDoc& doc) { return Node{ &doc.root, &doc }; }

    static JsonResult<JsonKind> Kind(Node node)
    {
        if (node.doc->visited++ == node.doc->failAtValue)
        {
            return nonstd::make_unexpected(std::string("injected failure"));
        }
        return toyjson::KindOf(*node.value);
    }
    static JsonResult<bool> AsBool(Node node) { return std::get<bool>(node.value->data); }
    static JsonResult<std::int64_t> AsInt(Node node) { return std::get<std::int64_t>(node.value->data); }
    static JsonResult<std::uint64_t> AsUInt(Node node) { return std::get<std::uint64_t>(node.value->data); }
    static JsonResult<double> AsDouble(Node node) { return std::get<double>(node.value->data); }
    static JsonResult<std::string> AsString(Node node)
    {
        const auto& text = std::get<std::string>(node.value->data);
        if (!node.doc->failOnString.empty() && text == node.doc->failOnString)
        {
            return nonstd::make_unexpected(std::string("unreadable string"));
        }
        return text;
    }
    static JsonResult<std::string_view> AsBigIntText(Node node) { return std::string_view(std::get<toyjson::BigIntText>(node.value->data).digits); }

    template<typename F>
    static JsonResult<void> ForEachMember(Node node, F onMember)
    {
        for (const auto& member : std::get<toyjson::Object>(node.value->data))
        {
            if (!onMember(std::string_view(member.key), Node{ &member.value, node.doc }))
            {
                break;
            }
            if (node.doc->failInObject)
            {
                return nonstd::make_unexpected(std::string("object cut short"));
            }
        }
        return {};
    }
    template<typename F>
    static JsonResult<void> ForEachElement(Node node, F onElement)
    {
        for (const auto& item : std::get<toyjson::Array>(node.value->data))
        {
            if (!onElement(Node{ &item, node.doc }))
            {
                break;
            }
        }
        return {};
    }

    static std::optional<std::uint64_t> Location(Node node) { return node.value->offset; }
};
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_TEST_BINDING_TOY_JSON_H
