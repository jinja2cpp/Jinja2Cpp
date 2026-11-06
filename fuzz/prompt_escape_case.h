// The property of AutoescapeMode::Prompt (docs/tasks/0358, docs/cpp-only-filters.md): data that the template does not
// trust never puts a special token in the output. Shared by the fuzz target fuzz_prompt_escape.cpp and the fixed-seed
// sample in test/filters_test.cpp.
//
// The input bytes choose a template from a grammar of the constructs that compose escaped values (output, `~`, `+`,
// format, `%`, join, macros, set blocks, loops, the escaping and HTML filters, filters that keep Markup) and the data
// strings. The specials are made of private-use code points U+E000-U+E007, which the template source never contains,
// and the separator is U+E00F, which neither the template nor the data contains: so every special in the output comes
// from the data, and the template cannot name the separator to remove it. Left out of the grammar are the ways a
// template author trusts or edits escaped text on purpose (`safe`, `{% autoescape false %}`, `neutralize` with
// arguments, slicing, `reverse` and `replace` of Markup), which the guarantee leaves to the author.
#ifndef JINJA2CPP_FUZZ_PROMPT_ESCAPE_CASE_H
#define JINJA2CPP_FUZZ_PROMPT_ESCAPE_CASE_H

#include <jinja2cpp/prompt.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace jinja2_fuzz
{

// One generated template, its data and the specials it must not let through
struct PromptEscapeCase
{
    std::string source; // UTF-8, without private-use code points
    jinja2::ValuesMap params;
    std::vector<std::string> specials;
};

// The outcome of rendering a case in one width
enum class PromptEscapeResult
{
    Clean,       // rendered; no special in the output
    NotRendered, // the template or its render failed, which the grammar allows (`+` of a list and a string)
    Leaked,      // a special in the output: the property fails
};

namespace detail
{
class ByteReader
{
public:
    ByteReader(const std::uint8_t* data, std::size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    // The next byte; zeros once the input ends, so every input is a case
    std::uint8_t Next() { return m_pos < m_size ? m_data[m_pos++] : 0; }
    std::size_t Pick(std::size_t count) { return count == 0 ? 0 : Next() % count; }
    [[nodiscard]] bool Done() const { return m_pos >= m_size; }

private:
    const std::uint8_t* m_data;
    std::size_t m_size;
    std::size_t m_pos = 0;
};

// U+E000 + index in UTF-8
inline std::string PrivateUse(unsigned index)
{
    const unsigned cp = 0xE000U + index;
    std::string result;
    result.push_back(static_cast<char>(0xE0U | (cp >> 12U)));
    result.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
    result.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    return result;
}

// Text of the template: printable ASCII without what starts a tag or ends a literal, so no private-use code point
// and nothing that changes the template's structure
inline std::string SourceText(ByteReader& in, std::size_t maxLength)
{
    static const char alphabet[] = "ab <>|/[]_-\n.x";
    std::string result;
    const std::size_t length = in.Pick(maxLength + 1);
    for (std::size_t idx = 0; idx < length; ++idx)
    {
        result.push_back(alphabet[in.Pick(sizeof(alphabet) - 1)]);
    }
    return result;
}

inline std::string Literal(ByteReader& in)
{
    return "'" + SourceText(in, 4) + "'";
}

class Generator
{
public:
    explicit Generator(ByteReader& in)
        : m_in(in)
    {
    }

    std::string Expression(int depth)
    {
        static const char* const variables[] = { "d0", "d1", "d2", "d3", "l[0]", "l[1]", "l|join", "m.k" };
        if (depth <= 0)
        {
            return m_in.Pick(3) == 0 ? Literal(m_in) : variables[m_in.Pick(8)];
        }
        const auto inner = [this, depth] { return Expression(depth - 1); };
        switch (m_in.Pick(26))
        {
        case 0:
            return Literal(m_in);
        case 1:
            return "(" + inner() + " ~ " + inner() + ")";
        case 2:
            return "(" + inner() + " ~ " + inner() + " ~ " + inner() + ")";
        case 3:
            return "(" + inner() + " + " + inner() + ")";
        case 4:
            return "(" + inner() + ")|e";
        case 5:
            return "(" + inner() + ")|forceescape";
        case 6:
            return "(" + inner() + ")|neutralize";
        case 7:
            return "(" + inner() + ")|" + std::string(m_in.Pick(2) == 0 ? "upper" : "lower");
        case 8:
            return "(" + inner() + ")|" + std::string(m_in.Pick(2) == 0 ? "trim" : "title");
        case 9:
            return "(" + inner() + ")|center(9)";
        case 10:
            return "(" + inner() + ")|truncate(" + std::to_string(3 + m_in.Pick(6)) + ", true, " + Literal(m_in) + ", 0)";
        case 11:
            return "'{}" + SourceText(m_in, 2) + "{}'.format(" + inner() + ", " + inner() + ")";
        case 12:
            return "'%s" + SourceText(m_in, 2) + "%.2s'|format(" + inner() + ", " + inner() + ")";
        case 13:
            return "('%s|%s' % (" + inner() + ", " + inner() + "))";
        case 14:
            return "[" + inner() + ", " + inner() + "]|join(" + Literal(m_in) + ")";
        case 15:
            return Literal(m_in) + ".join([" + inner() + ", " + inner() + "])";
        case 16:
            return "(" + inner() + ")|tojson";
        case 17:
            return "(" + inner() + ")|urlize";
        case 18:
            return "{'k': " + inner() + "}|xmlattr";
        case 19:
            return "(" + inner() + ")|fence";
        case 20:
            return "(" + inner() + " if d" + std::to_string(m_in.Pick(4)) + " else " + inner() + ")";
        case 21:
            return "mc(" + inner() + ")";
        case 22:
            return "d" + std::to_string(m_in.Pick(4)) + "|replace(" + Literal(m_in) + ", " + Literal(m_in) + ")";
        case 23:
            return "(" + inner() + ")|string";
        case 24:
            return "(" + inner() + ")|indent(2, true)";
        default:
            return "(" + inner() + ")|default(" + Literal(m_in) + ")";
        }
    }

    std::string Body(int depth)
    {
        std::string result;
        const std::size_t nodes = 1 + m_in.Pick(depth > 1 ? 6 : 3);
        for (std::size_t idx = 0; idx < nodes && !m_in.Done(); ++idx)
        {
            switch (depth <= 0 ? m_in.Pick(2) : m_in.Pick(9))
            {
            case 0:
                result += SourceText(m_in, 6);
                break;
            case 1:
            case 2:
            case 3:
                result += "{{ " + Expression(1 + static_cast<int>(m_in.Pick(3))) + " }}";
                break;
            case 4:
                result += "{% for c in d" + std::to_string(m_in.Pick(4)) + " %}{{ c }}" + SourceText(m_in, 1) + "{% endfor %}";
                break;
            case 5:
                result += "{% for v in l %}" + Body(depth - 1) + "{% endfor %}";
                break;
            case 6:
                result += "{% set s %}" + Body(depth - 1) + "{% endset %}{{ s }}{{ s ~ " + Expression(1) + " }}";
                break;
            case 7:
                result += "{% if d" + std::to_string(m_in.Pick(4)) + " %}" + Body(depth - 1) + "{% else %}" + Body(depth - 1) + "{% endif %}";
                break;
            default:
                // A runtime value: the volatile path of `~`
                result += "{% autoescape t %}" + Body(depth - 1) + "{% endautoescape %}";
                break;
            }
        }
        return result;
    }

private:
    ByteReader& m_in;
};
} // namespace detail

inline PromptEscapeCase MakePromptEscapeCase(const std::uint8_t* data, std::size_t size)
{
    detail::ByteReader in(data, size);
    PromptEscapeCase result;
    // One to four specials of two or three private-use code points from U+E000-U+E007
    const std::size_t specials = 1 + in.Pick(4);
    for (std::size_t idx = 0; idx < specials; ++idx)
    {
        std::string special;
        const std::size_t length = 2 + in.Pick(2);
        for (std::size_t unit = 0; unit < length; ++unit)
        {
            special += detail::PrivateUse(static_cast<unsigned>(in.Pick(8)));
        }
        result.specials.push_back(std::move(special));
    }
    // Data from an alphabet with the specials' code points, ASCII and a two-byte letter, and whole specials and their
    // halves, so that pieces of data can meet; the separator U+E00F is not in it
    auto dataString = [&in, &result] {
        static const char* const ascii[] = { "a", "<", ">", " ", "|", "\n", "\xC3\xA9" };
        std::string text;
        const std::size_t length = in.Pick(8);
        for (std::size_t idx = 0; idx < length; ++idx)
        {
            const auto pick = in.Pick(18);
            const auto& special = result.specials[in.Pick(result.specials.size())];
            const std::size_t half = 3 * (1 + in.Pick((special.size() / 3) - 1)); // whole code points, a proper part
            if (pick < 8)
            {
                text += detail::PrivateUse(static_cast<unsigned>(pick));
            }
            else if (pick < 15)
            {
                text += ascii[pick - 8];
            }
            else if (pick == 15)
            {
                text += special;
            }
            else
            {
                text += pick == 16 ? special.substr(0, half) : special.substr(half);
            }
        }
        return text;
    };
    for (const char* name : { "d0", "d1", "d2", "d3" })
    {
        result.params[name] = dataString();
    }
    result.params["l"] = jinja2::ValuesList{ dataString(), dataString() };
    result.params["m"] = jinja2::ValuesMap{ { "k", dataString() } };
    result.params["t"] = true;
    detail::Generator generator(in);
    result.source = "{% macro mc(p) %}<{{ p }}>{{ caller() if caller }}{% endmacro %}" + generator.Body(2);
    return result;
}

// Renders the case in prompt mode, narrow or wide, and checks the output for the specials
template<typename CharT>
PromptEscapeResult RenderPromptEscapeCase(const PromptEscapeCase& escapeCase)
{
    jinja2::TemplateEnv env;
    auto& settings = env.GetSettings();
    settings.autoescape = true;
    settings.autoescapeMode = jinja2::AutoescapeMode::Prompt;
    jinja2::PromptEscapeOptions options;
    options.specialTokens = escapeCase.specials;
    options.tags = { "document" };
    options.separator = detail::PrivateUse(0xF);
    settings.promptEscaper = jinja2::PromptEscaper(std::move(options));
    jinja2::BasicTemplate<CharT> tpl(&env);
    if (!tpl.Load(jinja2::ConvertString<std::basic_string<CharT>>(escapeCase.source)))
    {
        return PromptEscapeResult::NotRendered;
    }
    auto rendered = tpl.RenderAsString(escapeCase.params);
    if (!rendered)
    {
        return PromptEscapeResult::NotRendered;
    }
    const auto output = jinja2::ConvertString<std::string>(*rendered);
    for (const auto& special : escapeCase.specials)
    {
        if (output.find(special) != std::string::npos)
        {
            return PromptEscapeResult::Leaked;
        }
    }
    return PromptEscapeResult::Clean;
}

} // namespace jinja2_fuzz

#endif // JINJA2CPP_FUZZ_PROMPT_ESCAPE_CASE_H
