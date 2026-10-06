#ifndef JINJA2CPP_SRC_LOAD_SETTINGS_H
#define JINJA2CPP_SRC_LOAD_SETTINGS_H

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace jinja2::detail
{

// Delimiters and line prefixes from Settings, in the character type of a template
template<typename CharT>
struct Delimiters
{
    using string_t = std::basic_string<CharT>;

    // The begin delimiters the splitter tries at a position. The values are the splitter's match types
    // (TemplateParser::RM_*), so a match is made without a lookup
    enum class Begin : unsigned char
    {
        Var = 1,
        Block = 7,
        Comment = 9,
        LineStatement = 11,
        LineComment = 13
    };

    explicit Delimiters(const Settings& setts)
        : varBegin(Delimiter(setts.variableStartString, "{{"))
        , varEnd(Delimiter(setts.variableEndString, "}}"))
        , blockBegin(Delimiter(setts.blockStartString, "{%"))
        , blockEnd(Delimiter(setts.blockEndString, "%}"))
        , commentBegin(Delimiter(setts.commentStartString, "{#"))
        , commentEnd(Delimiter(setts.commentEndString, "#}"))
        , lineStatement(ConvertString<string_t>(setts.lineStatementPrefix))
        , lineComment(ConvertString<string_t>(setts.lineCommentPrefix))
    {
        // Jinja2 sorts the rules by length and then by token name, both descending
        begins.emplace_back(Begin::Var, &Delimiters::varBegin);
        if (!lineStatement.empty())
        {
            begins.emplace_back(Begin::LineStatement, &Delimiters::lineStatement);
        }
        if (!lineComment.empty())
        {
            begins.emplace_back(Begin::LineComment, &Delimiters::lineComment);
        }
        begins.emplace_back(Begin::Comment, &Delimiters::commentBegin);
        begins.emplace_back(Begin::Block, &Delimiters::blockBegin);
        std::stable_sort(begins.begin(), begins.end(), [this](auto& lhs, auto& rhs) { return (this->*lhs.second).size() > (this->*rhs.second).size(); });
        if (lineStatement.empty() && lineComment.empty())
        {
            for (const auto* begin : { &varBegin, &blockBegin, &commentBegin })
            {
                if (tagStarts.find(begin->front()) == string_t::npos)
                {
                    tagStarts.push_back(begin->front());
                }
            }
        }
    }

    string_t varBegin;
    string_t varEnd;
    string_t blockBegin;
    string_t blockEnd;
    string_t commentBegin;
    string_t commentEnd;
    string_t lineStatement;
    string_t lineComment;
    // Begin delimiters in the order Jinja2 tries them at one position: longest first
    std::vector<std::pair<Begin, string_t Delimiters::*>> begins;
    // The distinct first characters of the begin delimiters, or empty when line statement or
    // line comment prefixes are set (they can start at any line start or space)
    string_t tagStarts;

private:
    static string_t Delimiter(const std::string& value, const char* defaultValue)
    {
        return ConvertString<string_t>(value.empty() ? std::string(defaultValue) : value);
    }
};

// The settings a template is loaded and rendered with, and what Load derives from them. Immutable once made, so the
// templates of one environment share one copy for as long as the environment's settings stay the same
// (TemplateEnvImpl::GetLoadSettings), instead of each copying the settings and rebuilding the delimiters
struct LoadSettings
{
    explicit LoadSettings(const Settings& setts)
        : settings(setts)
        , narrow(setts)
        , wide(setts)
    {
    }

    template<typename CharT>
    [[nodiscard]] const Delimiters<CharT>& GetDelimiters() const
    {
        if constexpr (std::is_same_v<CharT, char>)
        {
            return narrow;
        }
        else
        {
            return wide;
        }
    }

    Settings settings;
    Delimiters<char> narrow;
    Delimiters<wchar_t> wide;
};

using LoadSettingsPtr = std::shared_ptr<const LoadSettings>;

// For templates made without an environment
const LoadSettingsPtr& DefaultLoadSettings();

} // namespace jinja2::detail

#endif // JINJA2CPP_SRC_LOAD_SETTINGS_H
