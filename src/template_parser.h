#ifndef JINJA2CPP_SRC_TEMPLATE_PARSER_H
#define JINJA2CPP_SRC_TEMPLATE_PARSER_H

#include "error_handling.h"
#include "expression_evaluator.h"
#include "expression_parser.h"
#include "helpers.h"
#include "internal_value.h"
#include "lexer.h"
#include "name_resolver.h"
#include "lexertk.h"
#include "load_settings.h"
#include "make_unexpected.h"
#include "node_arena.h"
#include "render_context.h"
#include "renderer.h"
#include "statements.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/container/small_vector.hpp>
#include <nonstd/expected.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2
{
template<typename CharT>
struct ParserTraits;

// Does what the Jinja2 lexer does to the source before tokenizing it: "\r\n", "\r" and
// "\n" all become "\n", and a single trailing newline is dropped unless
// keep_trailing_newline is set.
template<typename CharT>
void NormalizeTemplateNewlines(std::basic_string<CharT>& tpl, bool keepTrailingNewline)
{
    if (tpl.find(static_cast<CharT>('\r')) != std::basic_string<CharT>::npos)
    {
        size_t out = 0;
        for (size_t in = 0; in < tpl.size(); ++in)
        {
            auto ch = tpl[in];
            if (ch == '\r')
            {
                ch = '\n';
                if (in + 1 < tpl.size() && tpl[in + 1] == '\n')
                {
                    ++in;
                }
            }
            tpl[out++] = ch;
        }
        tpl.resize(out);
    }

    if (!keepTrailingNewline && !tpl.empty() && tpl.back() == '\n')
    {
        tpl.pop_back();
    }
}

struct KeywordsInfo
{
    MultiStringLiteral name;
    Keyword type;
};

struct TokenStrInfo : MultiStringLiteral
{
    template<typename CharT>
    auto GetName() const
    {
        return MultiStringLiteral::template GetValue<CharT>();
    }
};

template<typename T = void>
struct ParserTraitsBase
{
    static Token::Type s_keywords[];
    static KeywordsInfo s_keywordsInfo[43];
    static std::unordered_map<int, MultiStringLiteral> s_tokens;

    // Exact, case-sensitive match of an identifier against s_keywordsInfo. Called for every
    // identifier the lexer sees, so it looks in a table sorted once per character type, where
    // the first character picks a run of at most a dozen entries, instead of matching a regex
    // (which also cost a regex compilation per Load).
    template<typename CharT>
    static Keyword FindKeyword(std::basic_string_view<CharT> name)
    {
        struct Entry
        {
            std::basic_string_view<CharT> name;
            Keyword type;
        };
        static constexpr std::size_t charsCount = 128;
        struct Table
        {
            std::array<Entry, std::size(s_keywordsInfo)> entries;
            // Entries starting with character `c` are [runs[c], runs[c + 1])
            std::array<std::uint8_t, charsCount + 1> runs;
        };
        static const auto table = [] {
            Table result{};
            std::transform(std::begin(s_keywordsInfo), std::end(s_keywordsInfo), result.entries.begin(), [](const KeywordsInfo& info) {
                return Entry{ info.name.template GetCStr<CharT>(), info.type };
            });
            std::sort(result.entries.begin(), result.entries.end(), [](const Entry& lhs, const Entry& rhs) { return lhs.name < rhs.name; });
            std::size_t idx = 0;
            for (std::size_t ch = 0; ch <= charsCount; ++ch)
            {
                while (idx < result.entries.size() && static_cast<std::size_t>(result.entries[idx].name[0]) < ch)
                {
                    ++idx;
                }
                result.runs[ch] = static_cast<std::uint8_t>(idx);
            }
            return result;
        }();

        if (name.empty() || static_cast<std::make_unsigned_t<CharT>>(name[0]) >= charsCount)
        {
            return Keyword::Unknown;
        }
        const auto first = static_cast<std::size_t>(name[0]);
        const auto end = table.entries.begin() + table.runs[first + 1];
        auto entry = std::find_if(table.entries.begin() + table.runs[first], end, [name](const Entry& candidate) { return candidate.name == name; });
        return entry == end ? Keyword::Unknown : entry->type;
    }
};

template<>
struct ParserTraits<char> : public ParserTraitsBase<>
{
    static std::string GetAsString(const std::string& str, CharRange range) { return str.substr(range.startOffset, range.size()); }
    static InternalValue RangeToNum(const std::string& str, CharRange range, Token::Type hint)
    {
        // a fixed-size buffer overflowed on literals longer than 34 characters
        const std::string literal = str.substr(range.startOffset, range.size());
        const char* buff = literal.c_str();
        InternalValue result;
        if (hint == Token::IntegerNum)
        {
            result = InternalValue(static_cast<int64_t>(strtoll(buff, nullptr, 0)));
        }
        else
        {
            char* endBuff = nullptr;
            errno = 0; // a stale ERANGE from earlier code would turn every integer into a float
            int64_t val = strtoll(buff, &endBuff, 10);
            if ((errno == ERANGE) || *endBuff)
            {
                endBuff = nullptr;
                double dblVal = strtod(buff, nullptr);
                result = static_cast<double>(dblVal);
            }
            else
            {
                result = static_cast<int64_t>(val);
            }
        }
        return result;
    }
};

template<>
struct ParserTraits<wchar_t> : public ParserTraitsBase<>
{
    static std::string GetAsString(const std::wstring& str, CharRange range)
    {
        auto srcStr = str.substr(range.startOffset, range.size());
        return detail::StringConverter<std::wstring, std::string>::DoConvert(srcStr);
    }
    static InternalValue RangeToNum(const std::wstring& str, CharRange range, Token::Type hint)
    {
        // a fixed-size buffer overflowed on literals longer than 34 characters
        const std::wstring literal = str.substr(range.startOffset, range.size());
        const wchar_t* buff = literal.c_str();
        InternalValue result;
        if (hint == Token::IntegerNum)
        {
            result = static_cast<int64_t>(wcstoll(buff, nullptr, 0));
        }
        else
        {
            wchar_t* endBuff = nullptr;
            errno = 0; // a stale ERANGE from earlier code would turn every integer into a float
            int64_t val = wcstoll(buff, &endBuff, 10);
            if ((errno == ERANGE) || *endBuff)
            {
                endBuff = nullptr;
                double dblVal = wcstod(buff, nullptr);
                result = static_cast<double>(dblVal);
            }
            else
            {
                result = static_cast<int64_t>(val);
            }
        }
        return result;
    }
};

// What a `{% trans %}` block collects until its `{% endtrans %}` (Jinja2's i18n extension)
struct TransInfo
{
    // The parameters of the tag, then the names the message uses that are not parameters
    std::vector<std::pair<std::string, NodeRef<Expression>>> variables;
    size_t paramsCount = 0;
    // The message context string (pgettext), undefined if there is none
    InternalValue context;
    // The messages, `%` doubled and `{{ name }}` written as `%(name)s`, in the string type of the template
    TargetString singular;
    TargetString plural;
    std::vector<std::string> singularNames;
    std::vector<std::string> pluralNames;
    bool hasPlural = false;
    // The variable that selects the plural form, empty if none does yet
    std::string pluralVar;
    std::optional<bool> trimmed;

    [[nodiscard]] bool HasVariable(const std::string& name) const
    {
        return std::any_of(variables.begin(), variables.end(), [&name](auto& var) { return var.first == name; });
    }
    [[nodiscard]] bool HasParam(const std::string& name) const
    {
        return std::any_of(variables.begin(), variables.begin() + static_cast<std::ptrdiff_t>(paramsCount), [&name](auto& var) { return var.first == name; });
    }
};

struct StatementInfo
{
    enum Type
    {
        TemplateRoot,
        IfStatement,
        ElseIfStatement,
        ForStatement,
        SetStatement,
        ExtendsStatement,
        BlockStatement,
        MacroStatement,
        MacroCallStatement,
        WithStatement,
        FilterStatement,
        AutoescapeStatement,
        TransStatement
    };

    Type type{};
    // The body the statement's tags add to, made into a ComposedRenderer when the statement ends
    boost::container::small_vector<NodeRef<IRendererBase>, 8> body;
    Token token;
    NodeRef<IRendererBase> renderer;
    // Set on `{% trans %}` only
    std::shared_ptr<TransInfo> trans;
    // The resolver's frame of the body (0117 P1)
    NameResolver::FrameId frame = NameResolver::NoFrame;

    static StatementInfo Create(Type type, const Token& tok)
    {
        StatementInfo result;
        result.type = type;
        result.token = tok;
        return result;
    }
};

// The statements open at the current tag, the template's root first. Few are open at once,
// so the stack stays inline; no reference to an entry is kept across a push
using StatementInfoList = boost::container::small_vector<StatementInfo, 4>;

// What the template's root learns from its tags: the blocks it defines and whether it
// extends another. The parse gives them to its TemplateRenderer at the end
struct TemplateRootInfo
{
    boost::container::small_vector<NodeRef<BlockStatement>, 4> blocks;
    bool hasExtends = false;

    // False if a block of this name is defined already. Most templates define few blocks,
    // whose names are compared one by one; past those, the names go to a set, so that a
    // template of many blocks does not parse in quadratic time
    bool AddBlock(const NodeArena& nodes, NodeRef<BlockStatement> block)
    {
        // The arena never moves a node while it is built, so the names stay put
        const std::string_view name = nodes[block].GetName(nodes);
        if (blocks.size() < LinearLimit)
        {
            if (std::any_of(blocks.begin(), blocks.end(), [&nodes, name](NodeRef<BlockStatement> other) { return nodes[other].GetName(nodes) == name; }))
            {
                return false;
            }
        }
        else
        {
            if (!names)
            {
                names.emplace();
                for (const auto other : blocks)
                {
                    names->insert(nodes[other].GetName(nodes));
                }
            }
            if (!names->insert(name).second)
            {
                return false;
            }
        }
        blocks.push_back(block);
        return true;
    }

    static constexpr std::size_t LinearLimit = 16;
    // The names of the blocks, once there are LinearLimit of them. Made only then: even an
    // empty set costs every small template's parse a bucket clear when it is destroyed
    std::optional<std::unordered_set<std::string_view>> names;
};

// An assignment target, and whether it names `loop`, which a loop may not bind
struct ParsedTarget
{
    AssignTarget target;
    bool namesLoop = false;
};

class StatementsParser
{
public:
    using ParseResult = nonstd::expected<void, ParseError>;

    // A parser lives for one tag; the settings and the arena outlive it
    StatementsParser(const Settings& settings, TemplateEnv* env, NodeArena& nodes, NameResolver& names, TemplateRootInfo& root)
        : m_settings(settings)
        , m_env(env)
        , m_nodes(nodes)
        , m_names(names)
        , m_root(root)
    {
    }

    ParseResult Parse(LexScanner& lexer, StatementInfoList& statementsInfo);

private:
    ParseResult ParseNonKeywordStatement(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& tok);
    ParseResult ParseFor(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndFor(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseIf(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseElse(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseElIf(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndIf(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseSet(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndSet(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseBlock(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndBlock(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseExtends(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseMacro(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    nonstd::expected<MacroParamsInfo, ParseError> ParseMacroParams(LexScanner& lexer);
    ParseResult ParseEndMacro(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseCall(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndCall(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseInclude(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseImport(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseFrom(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseDo(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseWith(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndWith(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseFilter(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndFilter(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseLoopControl(StatementInfoList& statementsInfo, const Token& stmtTok, LoopControl control);
    nonstd::expected<ParsedTarget, ParseError> ParseAssignTarget(LexScanner& lexer, bool withNamespace);
    ParseResult ParseAutoescape(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndAutoescape(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseInTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParsePluralize(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);

    // The statement on top of the stack, taken off it
    static StatementInfo PopStatement(StatementInfoList& statementsInfo)
    {
        auto info = std::move(statementsInfo.back());
        statementsInfo.pop_back();
        return info;
    }
    // The statement's body, as a node
    NodeRef<ComposedRenderer> TakeBody(StatementInfo& info)
    {
        auto body = m_nodes.MakeWithItems<ComposedRenderer>(boost::span<const ComposedRenderer::Child>(info.body.data(), info.body.size()));
        info.body.clear();
        return body;
    }

    const Settings& m_settings;
    TemplateEnv* m_env;
    NodeArena& m_nodes;
    NameResolver& m_names;
    TemplateRootInfo& m_root;
};

// `{{ name }}` inside `{% trans %}`: Jinja2 allows a plain name only. The result is the name
class TransVariableParser
{
public:
    using ParseResult = nonstd::expected<std::string, ParseError>;

    TransVariableParser(const Settings&, TemplateEnv*, NodeArena&, NameResolver&) {}

    static ParseResult Parse(LexScanner& lexer)
    {
        auto tok = lexer.NextToken();
        if (tok != Token::Identifier)
        {
            return MakeParseError(ErrorCode::ExpectedIdentifier, tok);
        }
        auto next = lexer.NextToken();
        if (next != Token::Eof)
        {
            auto eof = next;
            eof.type = Token::Eof;
            return MakeParseError(ErrorCode::ExpectedToken, next, { eof });
        }
        return lexer.GetAsString(tok);
    }
};

template<typename CharT>
class TemplateParser : public LexerHelper
{
public:
    using string_t = std::basic_string<CharT>;
    using traits_t = ParserTraits<CharT>;
    using ErrorInfo = BasicErrorInfo<CharT>;
    using ParseResult = nonstd::expected<NodeRef<TemplateRenderer>, std::vector<ErrorInfo>>;

    // The tree goes to `nodes`
    TemplateParser(const string_t* tpl, const detail::LoadSettings& setts, TemplateEnv* env, const std::string& tplName, NodeArena& nodes)
        : m_template(tpl)
        , m_templateName(tplName)
        , m_settings(setts.settings)
        , m_env(env)
        , m_nodes(nodes)
        , m_delims(setts.GetDelimiters<CharT>())
        , m_metadataType(setts.settings.defaultMetadataType)
    {
    }
    ParseResult Parse()
    {
        auto roughResult = DoRoughParsing();

        if (!roughResult)
        {
            return ParseErrorsToErrorInfo(roughResult.error());
        }

        auto templateRenderer = m_nodes.Make<TemplateRenderer>();

        auto fineResult = DoFineParsing(templateRenderer);
        if (!fineResult)
        {
            return ParseErrorsToErrorInfo(fineResult.error());
        }
        m_names.Resolve(m_nodes);

        return templateRenderer;
    }

    // Frees what only the parse needed. Load calls it before Seal makes the tree's buffer,
    // so that the two are not held at once
    void ReleaseParseState()
    {
        std::vector<TextBlockInfo>().swap(m_textBlocks);
        m_lines.clear();
        m_lines.shrink_to_fit();
    }

    // What the names of the template resolved to (for the tests)
    [[nodiscard]] const NameResolver& GetNames() const { return m_names; }

    // Text the tree points to besides the source, null if none: kept as long as the tree.
    // A list, so that the texts do not move as more are added
    using ConvertedTexts = std::forward_list<string_t>;
    std::unique_ptr<ConvertedTexts> TakeConvertedTexts() { return std::move(m_convertedTexts); }

    MetadataInfo<CharT> GetMetadataInfo() const
    {
        MetadataInfo<CharT> result;
        result.metadataType = m_metadataType;
        result.metadata = m_metadata;
        result.location = m_metadataLocation;
        return result;
    }

private:
    enum
    {
        RM_Unknown,
        RM_ExprBegin,
        RM_ExprEnd,
        RM_RawBegin,
        RM_RawEnd,
        RM_MetaBegin,
        RM_MetaEnd,
        RM_StmtBegin,
        RM_StmtEnd,
        RM_CommentBegin,
        RM_CommentEnd,
        RM_LineStmtBegin,
        RM_LineStmtEnd,
        RM_LineComment
    };

    // A delimiter, a whole raw/meta tag or a line statement boundary found by the splitter
    struct RoughMatch
    {
        unsigned type = RM_Unknown;
        size_t start = 0;
        // Up to the `+`/`-` modifier of a delimiter; the whole tag for raw and meta tags
        size_t length = 0;
        // RM_LineStmtEnd: where the text after the line statement starts
        size_t resume = 0;
    };

    struct LineInfo
    {
        CharRange range;
        unsigned lineNumber;
    };

    enum class TextBlockType
    {
        RawText,
        Expr,
        Statement,
        Comment,
        LineStatement,
        RawBlock,
        MetaBlock
    };

    struct TextBlockInfo
    {
        CharRange range;
        TextBlockType type;
    };

    using Delimiters = detail::Delimiters<CharT>;
    static_assert(static_cast<unsigned>(Delimiters::Begin::Var) == RM_ExprBegin && static_cast<unsigned>(Delimiters::Begin::Block) == RM_StmtBegin
                      && static_cast<unsigned>(Delimiters::Begin::Comment) == RM_CommentBegin
                      && static_cast<unsigned>(Delimiters::Begin::LineStatement) == RM_LineStmtBegin
                      && static_cast<unsigned>(Delimiters::Begin::LineComment) == RM_LineComment,
                  "a begin delimiter's kind is the type of what it matches");

    nonstd::expected<void, std::vector<ParseError>> DoRoughParsing()
    {
        std::vector<ParseError> foundErrors;

        SplitLines();
        // Small templates have a few blocks: one allocation instead of growing from one
        m_textBlocks.reserve(16);

        m_currentBlockInfo.range.startOffset = 0;
        m_currentBlockInfo.range.endOffset = 0;
        m_currentBlockInfo.type = TextBlockType::RawText;
        size_t pos = 0;
        for (;;)
        {
            auto match = FindNextMatch(pos);
            if (match.type == RM_Unknown)
            {
                break;
            }
            auto result = ParseRoughMatch(match, pos);
            if (!result)
            {
                foundErrors.push_back(result.error());
                return MakeUnexpected(std::move(foundErrors));
            }
        }

        if (m_currentBlockInfo.type == TextBlockType::RawBlock)
        {
            nonstd::expected<void, ParseError> result =
                MakeParseError(ErrorCode::ExpectedRawEnd, MakeToken(Token::RawEnd, { m_template->size(), m_template->size() }));
            foundErrors.push_back(result.error());
            return MakeUnexpected(std::move(foundErrors));
        }
        if (m_currentBlockInfo.type == TextBlockType::MetaBlock)
        {
            nonstd::expected<void, ParseError> result =
                MakeParseError(ErrorCode::ExpectedMetaEnd, MakeToken(Token::RawEnd, { m_template->size(), m_template->size() }));
            foundErrors.push_back(result.error());
            return MakeUnexpected(std::move(foundErrors));
        }
        if (IsBlockLeftOpen())
        {
            // Jinja2: a `{{`, `{%` or `{#` left open at the end of the template is an error
            auto closing = Token::CommentEnd;
            if (m_currentBlockInfo.type == TextBlockType::Expr)
            {
                closing = Token::ExprEnd;
            }
            else if (m_currentBlockInfo.type == TextBlockType::Statement || m_currentBlockInfo.type == TextBlockType::LineStatement)
            {
                closing = Token::StmtEnd;
            }
            auto eof = m_template->size();
            nonstd::expected<void, ParseError> result =
                MakeParseError(ErrorCode::ExpectedToken, MakeToken(Token::Eof, { eof, eof }), { MakeToken(closing, { eof, eof }) });
            foundErrors.push_back(result.error());
            return MakeUnexpected(std::move(foundErrors));
        }

        PushCurrentBlock(m_template->size());

        if (!foundErrors.empty())
        {
            return MakeUnexpected(std::move(foundErrors));
        }
        return nonstd::expected<void, std::vector<ParseError>>();
    }

    void SplitLines()
    {
        auto& tpl = *m_template;
        size_t lineStart = 0;
        unsigned lineNumber = 0;
        for (auto pos = tpl.find('\n'); pos != string_t::npos; pos = tpl.find('\n', pos + 1))
        {
            m_lines.push_back(LineInfo{ { lineStart, pos }, lineNumber++ });
            lineStart = pos + 1;
        }
        m_lines.push_back(LineInfo{ { lineStart, tpl.size() }, lineNumber });
    }

    // The next delimiter that matters for the block the splitter is in, searching from `pos`
    RoughMatch FindNextMatch(size_t pos) const
    {
        switch (m_currentBlockInfo.type)
        {
        case TextBlockType::RawText:
            return FindTagInText(pos);
        case TextBlockType::Expr:
            return FindBlockEnd(pos, m_delims.varEnd, RM_ExprEnd);
        case TextBlockType::Statement:
            return FindBlockEnd(pos, m_delims.blockEnd, RM_StmtEnd);
        case TextBlockType::LineStatement:
            return FindBlockEnd(pos, string_t(), RM_LineStmtEnd);
        case TextBlockType::Comment:
        {
            // Jinja2 ends a comment at the first end delimiter, nested begin delimiters are text
            auto end = m_template->find(m_delims.commentEnd, pos);
            if (end != string_t::npos)
            {
                return MakeMatch(RM_CommentEnd, end, m_delims.commentEnd.size());
            }
            break;
        }
        case TextBlockType::RawBlock:
        case TextBlockType::MetaBlock:
            return FindRawOrMetaEnd(pos, m_currentBlockInfo.type == TextBlockType::RawBlock);
        }
        return RoughMatch();
    }

    // The first tag in plain text at or after `pos`
    RoughMatch FindTagInText(size_t pos) const
    {
        for (; pos < m_template->size(); ++pos)
        {
            // Without line prefixes a tag can only start on the first character of a begin
            // delimiter: jump there instead of trying every delimiter at every byte
            if (m_delims.tagStarts.size() == 1)
            {
                pos = m_template->find(m_delims.tagStarts[0], pos);
            }
            else if (!m_delims.tagStarts.empty())
            {
                pos = m_template->find_first_of(m_delims.tagStarts, pos);
            }
            if (pos == string_t::npos)
            {
                break;
            }
            auto match = MatchTagAt(pos);
            if (match.type != RM_Unknown)
            {
                return match;
            }
        }
        return RoughMatch();
    }

    // The `{% endraw %}` or `{% endmeta %}` tag that closes the current block
    RoughMatch FindRawOrMetaEnd(size_t pos, bool isRaw) const
    {
        for (pos = m_template->find(m_delims.blockBegin, pos); pos != string_t::npos; pos = m_template->find(m_delims.blockBegin, pos + 1))
        {
            auto length = isRaw ? MatchNamedTag(pos, "endraw", true) : MatchNamedTag(pos, "endmeta", false);
            if (length != 0)
            {
                return MakeMatch(isRaw ? RM_RawEnd : RM_MetaEnd, pos, length);
            }
        }
        return RoughMatch();
    }

    static RoughMatch MakeMatch(unsigned type, size_t start, size_t length, size_t resume = 0)
    {
        RoughMatch result;
        result.type = type;
        result.start = start;
        result.length = length;
        result.resume = resume;
        return result;
    }

    // A `{% raw %}`, `{% endraw %}`, `{% meta %}` or `{% endmeta %}` tag that starts at `pos`
    RoughMatch MatchRawOrMetaTagAt(size_t pos) const
    {
        auto& tpl = *m_template;
        // The first letter of the tag name rules out most of the four tags tried here
        auto word = pos + m_delims.blockBegin.size();
        if (word < tpl.size() && (tpl[word] == '-' || tpl[word] == '+'))
        {
            ++word;
        }
        while (word < tpl.size() && IsSpace(tpl[word]))
        {
            ++word;
        }
        const auto first = word < tpl.size() ? tpl[word] : CharT();
        // `endif`, `endfor`, `else` and the like: only `endraw` and `endmeta` go on
        if (first == 'e' && (word + 3 >= tpl.size() || (tpl[word + 3] != 'r' && tpl[word + 3] != 'm')))
        {
            return RoughMatch();
        }
        if (auto length = first == 'r' ? MatchNamedTag(pos, "raw", true, false) : 0)
        {
            return MakeMatch(RM_RawBegin, pos, length);
        }
        if (auto length = first == 'e' ? MatchNamedTag(pos, "endraw", true) : 0)
        {
            return MakeMatch(RM_RawEnd, pos, length);
        }
        if (auto length = first == 'm' ? MatchNamedTag(pos, "meta", false) : 0)
        {
            return MakeMatch(RM_MetaBegin, pos, length);
        }
        if (auto length = first == 'e' ? MatchNamedTag(pos, "endmeta", false) : 0)
        {
            return MakeMatch(RM_MetaEnd, pos, length);
        }
        return RoughMatch();
    }

    // A line statement prefix at `pos`. Jinja2: `^[ \t\v]*` + prefix
    RoughMatch MatchLineStmtBeginAt(size_t pos, const string_t& delimiter, bool lineStart) const
    {
        auto& tpl = *m_template;
        if (!lineStart)
        {
            return RoughMatch();
        }
        auto prefixPos = pos;
        while (prefixPos < tpl.size() && (tpl[prefixPos] == ' ' || tpl[prefixPos] == '\t' || tpl[prefixPos] == '\v'))
        {
            ++prefixPos;
        }
        if (IsAt(prefixPos, delimiter))
        {
            return MakeMatch(RM_LineStmtBegin, pos, prefixPos - pos + delimiter.size());
        }
        return RoughMatch();
    }

    // A line comment prefix at `pos`. Jinja2: `(?:^|(?<=\S))[^\S\r\n]*` + prefix, so the spaces before
    // the prefix go with the comment
    RoughMatch MatchLineCommentAt(size_t pos, const string_t& delimiter, bool lineStart) const
    {
        auto& tpl = *m_template;
        if (!lineStart && IsSpace(tpl[pos - 1]))
        {
            return RoughMatch();
        }
        auto prefixPos = pos;
        while (prefixPos < tpl.size() && IsSpace(tpl[prefixPos]) && tpl[prefixPos] != '\n' && tpl[prefixPos] != '\r')
        {
            ++prefixPos;
        }
        if (IsAt(prefixPos, delimiter))
        {
            return MakeMatch(RM_LineComment, pos, prefixPos - pos + delimiter.size());
        }
        return RoughMatch();
    }

    // A tag that starts at `pos` of the template text
    RoughMatch MatchTagAt(size_t pos) const
    {
        auto& tpl = *m_template;
        if (IsAt(pos, m_delims.blockBegin))
        {
            auto match = MatchRawOrMetaTagAt(pos);
            if (match.type != RM_Unknown)
            {
                return match;
            }
        }

        const bool lineStart = pos == 0 || tpl[pos - 1] == '\n';
        for (auto& begin : m_delims.begins)
        {
            auto& delimiter = m_delims.*begin.second;
            RoughMatch match;
            switch (begin.first)
            {
            case Delimiters::Begin::LineStatement:
                match = MatchLineStmtBeginAt(pos, delimiter, lineStart);
                break;
            case Delimiters::Begin::LineComment:
                match = MatchLineCommentAt(pos, delimiter, lineStart);
                break;
            default:
                if (IsAt(pos, delimiter))
                {
                    match = MakeMatch(static_cast<unsigned>(begin.first), pos, delimiter.size());
                }
                break;
            }
            if (match.type != RM_Unknown)
            {
                return match;
            }
        }
        return RoughMatch();
    }

    // The end of an expression, a statement or a line statement (an empty `end`). As Jinja2's lexer
    // does, end delimiters inside string literals or open brackets do not count. With unbalanced brackets
    // (an error either way) the first end delimiter outside strings ends the block, so the parser reports
    // what is wrong inside it.
    RoughMatch FindBlockEnd(size_t pos, const string_t& end, unsigned type, bool balanced = true) const
    {
        auto& tpl = *m_template;
        // Once brackets fail to balance the template is an error anyway; do not rescan to the end for every later tag
        balanced = balanced && !m_unbalancedBrackets;
        const auto start = pos;
        unsigned balance = 0;
        const CharT endFirst = end.empty() ? CharT('\n') : end[0];
        for (; pos <= tpl.size(); ++pos)
        {
            while (pos < tpl.size() && IsPlainBlockChar(tpl[pos], endFirst))
            {
                ++pos;
            }
            if (balance == 0)
            {
                auto match = MatchBlockEndAt(pos, end, type);
                if (match.type != RM_Unknown)
                {
                    return match;
                }
            }
            if (pos == tpl.size())
            {
                break;
            }
            pos = SkipStringOrCountBracket(pos, balanced, balance);
        }
        if (!balanced)
        {
            return RoughMatch();
        }
        m_unbalancedBrackets = true;
        return FindBlockEnd(start, end, type, false);
    }

    // Characters that neither end the block nor open or close anything, skipped in a tight loop
    static bool IsPlainBlockChar(CharT ch, CharT endFirst)
    {
        // One lookup for ASCII instead of a compare per special character
        static const auto special = [] {
            std::array<bool, 128> result{};
            for (char c : { '\'', '"', '(', ')', '[', ']', '{', '}', '\n' })
            {
                result[static_cast<unsigned char>(c)] = true;
            }
            return result;
        }();
        const auto code = static_cast<std::make_unsigned_t<CharT>>(ch);
        return ch != endFirst && (code >= special.size() || !special[code]);
    }

    // The end of a block of `type` at `pos`, outside any brackets
    RoughMatch MatchBlockEndAt(size_t pos, const string_t& end, unsigned type) const
    {
        auto& tpl = *m_template;
        if (type == RM_LineStmtEnd && (pos == tpl.size() || tpl[pos] == '\n'))
        {
            // Jinja2 ends a line statement with `\s*(\n|$)`: blank lines after it go too
            auto next = pos;
            while (next < tpl.size() && IsSpace(tpl[next]))
            {
                ++next;
            }
            if (next != tpl.size())
            {
                next = tpl.rfind('\n', next) + 1;
            }
            return MakeMatch(type, pos, 0, next);
        }
        if (type != RM_LineStmtEnd && IsAt(pos, end))
        {
            return MakeMatch(type, pos, end.size());
        }
        return RoughMatch();
    }

    // Steps over the string literal that opens at `pos` or counts the bracket there (when `balanced`);
    // returns the position the scan goes on from
    size_t SkipStringOrCountBracket(size_t pos, bool balanced, unsigned& balance) const
    {
        auto ch = (*m_template)[pos];
        if (ch == '\'' || ch == '"')
        {
            auto closing = FindStringEnd(pos);
            return closing != string_t::npos ? closing : pos;
        }
        if (!balanced)
        {
            return pos;
        }
        if (ch == '(' || ch == '[' || ch == '{')
        {
            ++balance;
        }
        else if ((ch == ')' || ch == ']' || ch == '}') && balance != 0)
        {
            --balance;
        }
        return pos;
    }

    // The closing quote of the string literal that opens at `pos`, or npos if it is not closed
    size_t FindStringEnd(size_t pos) const
    {
        auto& tpl = *m_template;
        auto quote = tpl[pos];
        // An unclosed string is an error anyway; after one, do not rescan to the end for every later quote
        bool& unclosed = m_unclosedString[quote == '"' ? 1 : 0];
        if (unclosed)
        {
            return string_t::npos;
        }
        for (++pos; pos < tpl.size(); ++pos)
        {
            if (tpl[pos] == '\\')
            {
                ++pos;
            }
            else if (tpl[pos] == quote)
            {
                return pos;
            }
        }
        unclosed = true;
        return string_t::npos;
    }

    // The length of the tag `<block begin>[-+]? name [-+]?<block end>` at `pos` (modifiers only where
    // `withModifiers` is set; Jinja2 does not allow `+` before the end of `raw`), or 0 if there is no such tag
    size_t MatchNamedTag(size_t pos, const char* name, bool withModifiers, bool plusAtEnd = true) const
    {
        auto& tpl = *m_template;
        if (!IsAt(pos, m_delims.blockBegin))
        {
            return 0;
        }
        auto cur = pos + m_delims.blockBegin.size();
        if (withModifiers && cur < tpl.size() && (tpl[cur] == '-' || tpl[cur] == '+'))
        {
            ++cur;
        }
        while (cur < tpl.size() && IsSpace(tpl[cur]))
        {
            ++cur;
        }
        for (; *name != '\0'; ++name, ++cur)
        {
            if (cur == tpl.size() || tpl[cur] != static_cast<CharT>(*name))
            {
                return 0;
            }
        }
        while (cur < tpl.size() && IsSpace(tpl[cur]))
        {
            ++cur;
        }
        if (withModifiers && cur < tpl.size() && (tpl[cur] == '-' || (plusAtEnd && tpl[cur] == '+')))
        {
            ++cur;
        }
        if (!IsAt(cur, m_delims.blockEnd))
        {
            return 0;
        }
        return cur + m_delims.blockEnd.size() - pos;
    }

    // The first character decides most calls; delimiters are short, so no compare call either
    bool IsAt(size_t pos, const string_t& str) const
    {
        const auto& tpl = *m_template;
        if (str.empty() || pos >= tpl.size() || tpl[pos] != str[0] || str.size() > tpl.size() - pos)
        {
            return false;
        }
        for (std::size_t idx = 1; idx < str.size(); ++idx)
        {
            if (tpl[pos + idx] != str[idx])
            {
                return false;
            }
        }
        return true;
    }

    static bool IsSpace(CharT ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v'; }

    nonstd::expected<void, ParseError> ParseRoughMatch(const RoughMatch& match, size_t& pos)
    {
        const auto matchEnd = match.start + match.length;
        pos = matchEnd;

        switch (match.type)
        {
        case RM_CommentBegin:
            StartControlBlock(TextBlockType::Comment, match.start, matchEnd, matchEnd);
            break;
        case RM_ExprBegin:
            StartControlBlock(TextBlockType::Expr, match.start, matchEnd, matchEnd);
            break;
        case RM_StmtBegin:
            StartControlBlock(TextBlockType::Statement, match.start, matchEnd, matchEnd);
            break;
        case RM_LineStmtBegin:
            StartControlBlock(TextBlockType::LineStatement, match.start, matchEnd, matchEnd);
            break;
        case RM_CommentEnd:
        case RM_ExprEnd:
        case RM_StmtEnd:
            pos = CloseControlBlock(match.start, match.length);
            break;
        case RM_LineStmtEnd:
            PushCurrentBlock(match.start);
            pos = m_currentBlockInfo.range.startOffset = match.resume;
            break;
        case RM_LineComment:
        {
            // The comment runs to the end of the line; the newline stays in the text
            StartControlBlock(TextBlockType::Comment, match.start, matchEnd, matchEnd);
            auto lineEnd = std::min(m_template->find('\n', matchEnd), m_template->size());
            PushCurrentBlock(lineEnd);
            pos = m_currentBlockInfo.range.startOffset = lineEnd;
            break;
        }
        case RM_RawBegin:
            StartControlBlock(TextBlockType::RawBlock, match.start, match.start + m_delims.blockBegin.size(), matchEnd);
            break;
        case RM_RawEnd:
            if (m_currentBlockInfo.type != TextBlockType::RawBlock)
            {
                return MakeParseError(ErrorCode::UnexpectedRawEnd, MakeToken(Token::RawEnd, { match.start, matchEnd }));
            }
            pos = CloseRawBlock(match);
            break;
        case RM_MetaBegin:
            if (m_hasMetaBlock)
            {
                return MakeParseError(ErrorCode::UnexpectedMetaBegin, MakeToken(Token::MetaBegin, { match.start, matchEnd }));
            }
            StartControlBlock(TextBlockType::MetaBlock, match.start, match.start + m_delims.blockBegin.size(), matchEnd);
            OffsetToLinePos(match.start, m_metadataLocation.line, m_metadataLocation.col);
            m_metadataLocation.fileName = m_templateName;
            break;
        case RM_MetaEnd:
            if (m_currentBlockInfo.type != TextBlockType::MetaBlock)
            {
                return MakeParseError(ErrorCode::UnexpectedMetaEnd, MakeToken(Token::MetaEnd, { match.start, matchEnd }));
            }
            pos = CloseRawBlock(match);
            m_hasMetaBlock = true;
            break;
        }

        return nonstd::expected<void, ParseError>();
    }

    // Ends the text before a tag at `matchStart` and opens the tag's block. The `+`/`-` modifier is at
    // `ctrlCharPos`; the block's content starts at `startOffset` (after the modifier, if any, except for raw and meta tags).
    void StartControlBlock(TextBlockType blockType, size_t matchStart, size_t ctrlCharPos, size_t startOffset)
    {
        // lstrip_blocks does not apply to expressions
        auto endOffset = StripBlockLeft(m_currentBlockInfo, ctrlCharPos, matchStart, blockType == TextBlockType::Expr ? false : m_settings.lstripBlocks);
        PushCurrentBlock(endOffset);

        if (startOffset < m_template->size() && blockType != TextBlockType::MetaBlock && blockType != TextBlockType::RawBlock)
        {
            if ((*m_template)[startOffset] == '+' || (*m_template)[startOffset] == '-')
            {
                ++startOffset;
            }
        }

        m_currentBlockInfo.type = blockType;

        // Jinja2 does not apply trim_blocks to the newline after `{% raw %}`, only `-%}` strips there
        if (blockType == TextBlockType::RawBlock)
        {
            startOffset = StripBlockRight(startOffset - m_delims.blockEnd.size(), m_delims.blockEnd.size(), false);
        }

        m_currentBlockInfo.range.startOffset = startOffset;
    }

    // Closes the expression, statement or comment whose end delimiter is at `endPos` and returns where the text after it starts
    size_t CloseControlBlock(size_t endPos, size_t endLength)
    {
        // trim_blocks does not apply to expressions
        auto next = StripBlockRight(endPos, endLength, m_currentBlockInfo.type == TextBlockType::Expr ? false : m_settings.trimBlocks);
        auto contentEnd = endPos;
        if (endPos > m_currentBlockInfo.range.startOffset && ((*m_template)[endPos - 1] == '+' || (*m_template)[endPos - 1] == '-'))
        {
            --contentEnd;
        }
        PushCurrentBlock(contentEnd);
        m_currentBlockInfo.range.startOffset = next;
        return next;
    }

    // Closes a raw or meta block at its end tag and returns where the text after it starts
    size_t CloseRawBlock(const RoughMatch& match)
    {
        auto endDelimiterPos = match.start + match.length - m_delims.blockEnd.size();
        auto contentEnd = StripBlockLeft(m_currentBlockInfo, match.start + m_delims.blockBegin.size(), match.start, m_settings.lstripBlocks);
        auto next = StripBlockRight(endDelimiterPos, m_delims.blockEnd.size(), m_settings.trimBlocks);
        PushCurrentBlock(contentEnd);
        m_currentBlockInfo.range.startOffset = next;
        return next;
    }

    void PushCurrentBlock(size_t endOffset)
    {
        m_currentBlockInfo.range.endOffset = endOffset;
        m_textBlocks.push_back(m_currentBlockInfo);
        m_currentBlockInfo.type = TextBlockType::RawText;
    }

    // `-` strips all whitespace after the block, newlines included; otherwise trim_blocks
    // removes one newline that directly follows it, and `+` disables trim_blocks.
    size_t StripBlockRight(size_t position, size_t endLength, bool trimBlocks)
    {
        bool doTotalStrip = false;

        size_t newPos = position + endLength;

        if (m_currentBlockInfo.type != TextBlockType::RawText && position > m_currentBlockInfo.range.startOffset)
        {
            auto ctrlChar = (*m_template)[position - 1];
            doTotalStrip = ctrlChar == '-';
            if (ctrlChar == '+')
            {
                trimBlocks = false;
            }
        }

        if (doTotalStrip)
        {
            auto locale = std::locale();
            while (newPos < m_template->size() && std::isspace((*m_template)[newPos], locale))
            {
                ++newPos;
            }
        }
        else if (trimBlocks && newPos < m_template->size() && (*m_template)[newPos] == '\n')
        {
            ++newPos;
        }
        return newPos;
    }

    size_t StripBlockLeft(TextBlockInfo& currentBlockInfo, size_t ctrlCharPos, size_t endOffset, bool doStrip)
    {
        bool doTotalStrip = false;
        if (ctrlCharPos < m_template->size())
        {
            auto ctrlChar = (*m_template)[ctrlCharPos];
            if (ctrlChar == '+')
            {
                doStrip = false;
            }
            else
            {
                doTotalStrip = ctrlChar == '-';
            }

            doStrip |= doTotalStrip;
        }
        if (!doStrip || (currentBlockInfo.type != TextBlockType::RawText && currentBlockInfo.type != TextBlockType::RawBlock))
        {
            return endOffset;
        }

        auto locale = std::locale();
        auto& tpl = *m_template;
        auto originalOffset = endOffset;
        bool sameLine = true;
        for (; endOffset != currentBlockInfo.range.startOffset && endOffset > 0; --endOffset)
        {
            auto ch = tpl[endOffset - 1];
            if (!std::isspace(ch, locale))
            {
                if (!sameLine)
                {
                    break;
                }

                return doTotalStrip ? endOffset : originalOffset;
            }
            if (ch == '\n')
            {
                if (!doTotalStrip)
                {
                    break;
                }
                sameLine = false;
            }
        }
        // lstrip_blocks strips only when the tag starts its line: the text may begin mid-line after another tag
        if (!doTotalStrip && endOffset != 0 && endOffset == currentBlockInfo.range.startOffset && tpl[endOffset - 1] != '\n')
        {
            return originalOffset;
        }
        return endOffset;
    }

    string_t ApplyNewlineSequence(const CharT* text, size_t size) const
    {
        string_t result;
        result.reserve(size);
        for (auto ch : std::basic_string_view<CharT>(text, size))
        {
            if (ch == '\n')
            {
                result.append(m_settings.newlineSequence.begin(), m_settings.newlineSequence.end());
            }
            else
            {
                result.push_back(ch);
            }
        }
        return result;
    }

    // Text is rendered straight from the template source unless newlines must become
    // newline_sequence, which needs a converted copy: the template keeps those with its
    // source (TakeConvertedTexts)
    NodeRef<RawTextRenderer> MakeRawTextRenderer(const CharRange& range)
    {
        const CharT* text = m_template->data() + range.startOffset;
        if (m_settings.newlineSequence == "\n" || std::find(text, text + range.size(), '\n') == text + range.size())
        {
            return m_nodes.Make<RawTextRenderer>(text, range.size());
        }

        if (!m_convertedTexts)
        {
            m_convertedTexts = std::make_unique<ConvertedTexts>();
        }
        const auto& converted = m_convertedTexts->emplace_front(ApplyNewlineSequence(text, range.size()));
        return m_nodes.Make<RawTextRenderer>(converted.data(), converted.size());
    }

    // The message of a `{% trans %}` block that the text goes to: the plural one after `{% pluralize %}`
    static string_t& TransMessage(TransInfo& trans)
    {
        auto& message = trans.hasPlural ? trans.plural : trans.singular;
        if (!std::holds_alternative<string_t>(message))
        {
            message = string_t();
        }
        return std::get<string_t>(message);
    }

    // Jinja2 makes the text of a trans block a format string: `%` is doubled
    void AppendTransText(TransInfo& trans, const CharRange& range)
    {
        auto& message = TransMessage(trans);
        for (auto ch : ApplyNewlineSequence(m_template->data() + range.startOffset, range.size()))
        {
            message.push_back(ch);
            if (ch == '%')
            {
                message.push_back(ch);
            }
        }
    }

    static void AppendTransVariable(TransInfo& trans, const std::string& name)
    {
        auto placeholder = "%(" + name + ")s";
        TransMessage(trans).append(placeholder.begin(), placeholder.end());
        (trans.hasPlural ? trans.pluralNames : trans.singularNames).push_back(name);
    }

    nonstd::expected<void, std::vector<ParseError>> DoFineParsing(NodeRef<TemplateRenderer> templateRef)
    {
        std::vector<ParseError> errors;
        StatementInfoList statementsStack;
        StatementInfo root = StatementInfo::Create(StatementInfo::TemplateRoot, Token());
        root.frame = m_names.PushUnit(NameResolver::NoFrame, templateRef);
        statementsStack.push_back(std::move(root));
        m_openStatements = &statementsStack;
        for (auto& origBlock : m_textBlocks)
        {
            auto& block = origBlock;

            switch (block.type)
            {
            case TextBlockType::RawBlock:
            case TextBlockType::RawText:
                FineParseRawText(block, statementsStack, errors);
                break;
            case TextBlockType::MetaBlock:
                FineParseMetaBlock(block);
                break;
            case TextBlockType::Expr:
                FineParseExpression(block, statementsStack, errors);
                break;
            case TextBlockType::Statement:
            case TextBlockType::LineStatement:
                if (!FineParseStatement(block, statementsStack, errors))
                {
                    m_openStatements = nullptr;
                    return MakeUnexpected(std::move(errors));
                }
                break;
            default:
                break;
            }
        }
        m_openStatements = nullptr;

        // Jinja2: a block statement left open at the end of the template is an error
        if (errors.empty() && statementsStack.size() > 1)
        {
            auto eof = m_template->size();
            auto& open = statementsStack.back();
            errors.push_back(
                MakeParseError(ErrorCode::ExpectedToken, MakeToken(Token::Eof, { eof, eof }), { MakeToken(GetEndToken(statementsStack), open.token.range) })
                    .error());
        }

        if (!errors.empty())
        {
            return MakeUnexpected(std::move(errors));
        }

        const auto& rootBody = statementsStack.front().body;
        auto& templateRoot = m_nodes[templateRef];
        templateRoot.SetBlocks(m_nodes.MakeSpan(m_root.blocks));
        if (m_root.hasExtends)
        {
            templateRoot.SetHasExtends();
        }
        templateRoot.SetBody(m_nodes.MakeWithItems<ComposedRenderer>(boost::span<const ComposedRenderer::Child>(rootBody.data(), rootBody.size())));
        return nonstd::expected<void, std::vector<ParseError>>();
    }

    void FineParseRawText(const TextBlockInfo& block, StatementInfoList& statementsStack, std::vector<ParseError>& errors)
    {
        auto range = block.range;
        if (range.size() == 0)
        {
            return;
        }
        if (IsInRequiredBlock(statementsStack) && !IsWhitespace(range))
        {
            errors.push_back(MakeParseError(ErrorCode::UnexpectedToken, MakeToken(Token::Identifier, range)).error());
            return;
        }
        if (statementsStack.back().type == StatementInfo::TransStatement)
        {
            AppendTransText(*statementsStack.back().trans, range);
            return;
        }
        statementsStack.back().body.push_back(MakeRawTextRenderer(range));
    }

    void FineParseMetaBlock(const TextBlockInfo& block)
    {
        auto range = block.range;
        if (range.size() == 0)
        {
            return;
        }
        auto metadata = std::basic_string_view<CharT>(m_template->data() + range.startOffset, range.size());
        if (!boost::algorithm::all(metadata, boost::algorithm::is_space()))
        {
            m_metadata = metadata;
        }
    }

    void FineParseExpression(const TextBlockInfo& block, StatementInfoList& statementsStack, std::vector<ParseError>& errors)
    {
        if (IsInRequiredBlock(statementsStack))
        {
            errors.push_back(MakeParseError(ErrorCode::UnexpectedToken, MakeToken(Token::Identifier, block.range)).error());
            return;
        }
        if (statementsStack.back().type == StatementInfo::TransStatement)
        {
            auto name = InvokeParser<std::string, TransVariableParser>(block);
            if (name)
            {
                AppendTransVariable(*statementsStack.back().trans, *name);
            }
            else
            {
                errors.push_back(name.error());
            }
            return;
        }
        m_names.SetCurrent(statementsStack.back().frame);
        auto parseResult = InvokeParser<NodeRef<IRendererBase>, ExpressionParser>(block);
        if (parseResult)
        {
            statementsStack.back().body.push_back(*parseResult);
        }
        else
        {
            errors.push_back(parseResult.error());
        }
    }

    // False when parsing has to stop here
    bool FineParseStatement(const TextBlockInfo& block, StatementInfoList& statementsStack, std::vector<ParseError>& errors)
    {
        auto parseResult = InvokeParser<void, StatementsParser>(block, statementsStack);
        if (!parseResult)
        {
            errors.push_back(parseResult.error());
            // Past the block nesting limit every later statement fails the same way
            if (parseResult.error().errorCode == ErrorCode::RecursionLimitExceeded)
            {
                return false;
            }
        }
        return true;
    }

    // Whether the template ends inside `{{`, `{%` or `{#`. Jinja2 drops a `{#` (or `{#-`, `{#+`)
    // that ends the template instead of reporting it unclosed.
    bool IsBlockLeftOpen() const
    {
        switch (m_currentBlockInfo.type)
        {
        case TextBlockType::Expr:
        case TextBlockType::Statement:
        case TextBlockType::LineStatement:
            return true;
        case TextBlockType::Comment:
            return m_currentBlockInfo.range.startOffset < m_template->size();
        default:
            return false;
        }
    }

    // The tag that closes the innermost open statement
    static Token::Type GetEndToken(const StatementInfoList& statementsStack)
    {
        auto p = statementsStack.rbegin();
        // `else` and `elif` are closed by the tag of the statement they continue
        while (p->type == StatementInfo::ElseIfStatement && std::next(p) != statementsStack.rend())
        {
            ++p;
        }
        switch (p->type)
        {
        case StatementInfo::IfStatement:
            return Token::EndIf;
        case StatementInfo::ForStatement:
            return Token::Endfor;
        case StatementInfo::SetStatement:
            return Token::EndSet;
        case StatementInfo::BlockStatement:
            return Token::EndBlock;
        case StatementInfo::MacroStatement:
            return Token::EndMacro;
        case StatementInfo::MacroCallStatement:
            return Token::EndCall;
        case StatementInfo::WithStatement:
            return Token::EndWith;
        case StatementInfo::FilterStatement:
            return Token::EndFilter;
        case StatementInfo::AutoescapeStatement:
            return Token::EndAutoescape;
        default:
            return Token::Eof;
        }
    }

    // Jinja2: required blocks can only contain comments or whitespace
    bool IsInRequiredBlock(const StatementInfoList& statementsStack) const
    {
        const auto& info = statementsStack.back();
        return info.type == StatementInfo::BlockStatement && m_nodes.Get<BlockStatement>(info.renderer).IsRequired();
    }

    bool IsWhitespace(const CharRange& range) const
    {
        auto begin = m_template->data() + range.startOffset;
        return std::all_of(begin, begin + range.size(), [](CharT ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v'; });
    }

    struct LexBuffers
    {
        // Room for the tokens of a typical tag, so the first tags do not grow the lists one token at a time
        LexBuffers()
        {
            tokenizer.reserve(16);
            tokens.reserve(16);
        }

        lexertk::generator<CharT> tokenizer;
        Lexer::TokensList tokens;
    };

    template<typename R, typename P, typename... Args>
    nonstd::expected<R, ParseError> InvokeParser(const TextBlockInfo& block, Args&&... args)
    {
        // Each tag lexes into the buffers of the previous one and reuses their capacity; a
        // nested call, should a parser ever make one, gets buffers of its own
        auto buffers = m_lexBuffers ? std::move(m_lexBuffers) : std::make_unique<LexBuffers>();
        auto result = ParseTag<R, P>(block, *buffers, std::forward<Args>(args)...);
        m_lexBuffers = std::move(buffers);
        return result;
    }

    template<typename R, typename P, typename... Args>
    nonstd::expected<R, ParseError> ParseTag(const TextBlockInfo& block, LexBuffers& buffers, Args&&... args)
    {
        auto& tokenizer = buffers.tokenizer;
        auto range = block.range;
        auto start = m_template->data();
        if (!tokenizer.process(start + range.startOffset, start + range.endOffset))
        {
            return MakeParseError(ErrorCode::Unspecified, MakeToken(Token::Unknown, { range.startOffset, range.startOffset + 1 }));
        }

        tokenizer.begin();
        Lexer lexer(
            [&tokenizer, adjust = range.startOffset]() mutable {
                lexertk::token tok = tokenizer.next_token();
                tok.position += adjust;
                return tok;
            },
            this,
            std::move(buffers.tokens));

        if (!lexer.Preprocess())
        {
            return MakeParseError(ErrorCode::Unspecified, MakeToken(Token::Unknown, { range.startOffset, range.startOffset + 1 }));
        }

        MarkMacroSpecialNames(lexer.GetTokens(), std::is_same_v<P, StatementsParser>);

        auto praser = MakeParser<P>();
        LexScanner scanner(lexer);
        auto result = praser.Parse(scanner, std::forward<Args>(args)...);
        buffers.tokens = lexer.ReleaseTokens();
        if (!result)
        {
            return MakeUnexpected(result.error());
        }

        return result;
    }

    // A parser for one tag: a statement's adds the blocks it defines to the root's
    template<typename P>
    P MakeParser()
    {
        if constexpr (std::is_same_v<P, StatementsParser>)
        {
            return P(m_settings, m_env, m_nodes, m_names, m_root);
        }
        else
        {
            return P(m_settings, m_env, m_nodes, m_names);
        }
    }

    // Tells the enclosing macros and call blocks which of `caller`, `varargs` and `kwargs`
    // their bodies use before assigning them. Jinja2 decides the same with find_undeclared,
    // which visits assignment targets and parameters before the values; so does this scan,
    // in source order, block by block.
    void MarkMacroSpecialNames(const Lexer::TokensList& tokens, bool isStatement)
    {
        if (!m_openStatements || tokens.empty())
        {
            return;
        }
        // Only an enclosing macro or call block takes the marks
        if (std::none_of(m_openStatements->begin(), m_openStatements->end(), [](const StatementInfo& info) { return IsMacroScope(info); }))
        {
            return;
        }

        // Most tags name none of them
        if (std::none_of(tokens.begin(), tokens.end(), [this](const Token& tok) { return SpecialNameOf(tok) != 0 || IsCallerString(tok); }))
        {
            return;
        }

        // Assignment targets of set/for/with and parameter names of nested macros and call blocks
        std::vector<bool> isStore(tokens.size(), false);
        auto keyword = isStatement ? tokens[0].keyword : Keyword::Unknown;
        switch (keyword)
        {
        case Keyword::Set:
            MarkSetTargets(tokens, isStore);
            break;
        case Keyword::For:
            MarkForTargets(tokens, isStore);
            break;
        case Keyword::With:
            MarkWithTargets(tokens, isStore);
            break;
        case Keyword::Macro:
        case Keyword::Call:
            MarkMacroParamNames(tokens, keyword == Keyword::Macro ? 2 : 1, isStore);
            break;
        default:
            break;
        }

        unsigned stores = 0;
        unsigned loads = 0;
        CollectSpecialNameUses(tokens, isStore, stores, loads);

        if ((stores | loads) == 0)
        {
            return;
        }

        for (auto& info : *m_openStatements)
        {
            if (!IsMacroScope(info))
            {
                continue;
            }
            auto& macro = m_nodes.Get<MacroStatement>(info.renderer);
            macro.DiscardSpecialNames(stores);
            macro.AddSpecialNames(loads);
        }
    }

    static bool IsMacroScope(const StatementInfo& info)
    {
        return info.type == StatementInfo::MacroStatement || info.type == StatementInfo::MacroCallStatement;
    }

    // Compares the source text, which for an identifier is its name, without copying it
    bool TokenTextIs(const Token& tok, std::string_view name) const
    {
        const auto* text = m_template->data() + tok.range.startOffset;
        return tok.range.size() == name.size() && std::equal(name.begin(), name.end(), text, [](char lhs, CharT rhs) { return static_cast<CharT>(lhs) == rhs; });
    }

    // The MacroStatement::Uses* flag of `caller`, `varargs` or `kwargs`; 0 for any other token
    unsigned SpecialNameOf(const Token& tok) const
    {
        if (tok.type != Token::Identifier)
        {
            return 0;
        }
        if (TokenTextIs(tok, "caller"))
        {
            return MacroStatement::UsesCaller;
        }
        if (TokenTextIs(tok, "varargs"))
        {
            return MacroStatement::UsesVarargs;
        }
        if (TokenTextIs(tok, "kwargs"))
        {
            return MacroStatement::UsesKwargs;
        }
        return 0;
    }

    static bool IsCallerString(const Token& tok) { return tok.type == Token::String && AsString(tok.value) == "caller"; }

    static void MarkSetTargets(const Lexer::TokensList& tokens, std::vector<bool>& isStore)
    {
        for (std::size_t idx = 1; idx < tokens.size() && tokens[idx] != Token::Assign && tokens[idx] != '|'; ++idx)
        {
            isStore[idx] = tokens[idx].type == Token::Identifier;
        }
    }

    static void MarkForTargets(const Lexer::TokensList& tokens, std::vector<bool>& isStore)
    {
        for (std::size_t idx = 1; idx < tokens.size() && tokens[idx] != Token::In && tokens[idx].keyword != Keyword::In; ++idx)
        {
            isStore[idx] = tokens[idx].type == Token::Identifier;
        }
    }

    static void MarkWithTargets(const Lexer::TokensList& tokens, std::vector<bool>& isStore)
    {
        // Targets are top-level `name =`; deeper ones are keyword arguments of a call
        int depth = 0;
        for (std::size_t idx = 1; idx + 1 < tokens.size(); ++idx)
        {
            const auto& tok = tokens[idx];
            if (tok == '(' || tok == '[' || tok == '{')
            {
                ++depth;
            }
            else if (tok == ')' || tok == ']' || tok == '}')
            {
                --depth;
            }
            isStore[idx] = depth == 0 && tok.type == Token::Identifier && tokens[idx + 1] == Token::Assign;
        }
    }

    // Parameter names of a nested macro or call block whose parameter list opens at `idx`
    static void MarkMacroParamNames(const Lexer::TokensList& tokens, std::size_t idx, std::vector<bool>& isStore)
    {
        if (idx >= tokens.size() || tokens[idx] != '(')
        {
            return;
        }
        int depth = 0;
        for (; idx < tokens.size(); ++idx)
        {
            const auto& tok = tokens[idx];
            if (tok == '(' || tok == '[' || tok == '{')
            {
                ++depth;
            }
            else if (tok == ')' || tok == ']' || tok == '}')
            {
                --depth;
            }
            else if (depth == 1 && tok.type == Token::Identifier && (tokens[idx - 1] == '(' || tokens[idx - 1] == ','))
            {
                isStore[idx] = true;
            }
            if (depth == 0)
            {
                break;
            }
        }
    }

    // Which special names the tag assigns (`stores`) and which it reads (`loads`)
    void CollectSpecialNameUses(const Lexer::TokensList& tokens, const std::vector<bool>& isStore, unsigned& stores, unsigned& loads) const
    {
        for (std::size_t idx = 0; idx < tokens.size(); ++idx)
        {
            const auto& tok = tokens[idx];
            if (isStore[idx])
            {
                stores |= SpecialNameOf(tok);
                continue;
            }
            // The `applymacro` filter takes the macro by name: `map('applymacro', macro='caller')`
            if (IsCallerString(tok))
            {
                loads |= MacroStatement::UsesCaller;
            }
            // Neither an attribute (`x.caller`) nor a keyword argument name (`f(caller=...)`)
            if (idx > 0 && tokens[idx - 1] == '.')
            {
                continue;
            }
            if (idx + 1 < tokens.size() && tokens[idx + 1] == Token::Assign)
            {
                continue;
            }
            loads |= SpecialNameOf(tok);
        }
    }

    nonstd::unexpected_type<std::vector<ErrorInfo>> ParseErrorsToErrorInfo(const std::vector<ParseError>& errors)
    {
        // Only the first error reaches the caller (Template::Load; Jinja2 also stops at the first
        // syntax error). Describing every error cost O(errors * line length) on one long line.
        std::vector<ErrorInfo> resultErrors;
        if (!errors.empty())
        {
            resultErrors.push_back(ParseErrorToErrorInfo(errors.front()));
        }

        return MakeUnexpected(std::move(resultErrors));
    }

    ErrorInfo ParseErrorToErrorInfo(const ParseError& e)
    {
        typename ErrorInfo::Data errInfoData;
        errInfoData.code = e.errorCode;
        errInfoData.srcLoc.fileName = m_templateName;
        OffsetToLinePos(e.errorToken.range.startOffset, errInfoData.srcLoc.line, errInfoData.srcLoc.col);
        errInfoData.locationDescr = GetLocationDescr(errInfoData.srcLoc.line, errInfoData.srcLoc.col);
        errInfoData.extraParams.emplace_back(TokenToString(e.errorToken));
        for (const auto& tok : e.relatedTokens)
        {
            errInfoData.extraParams.emplace_back(TokenToString(tok));
            if (tok.range.startOffset != e.errorToken.range.startOffset)
            {
                SourceLocation relLoc;
                relLoc.fileName = m_templateName;
                OffsetToLinePos(tok.range.startOffset, relLoc.line, relLoc.col);
                errInfoData.relatedLocs.push_back(std::move(relLoc));
            }
        }

        return ErrorInfo(std::move(errInfoData));
    }

    Token MakeToken(Token::Type type, const CharRange& range, string_t value = string_t())
    {
        Token tok;
        tok.type = type;
        tok.range = range;
        tok.value = TargetString(static_cast<string_t>(std::move(value)));

        return tok;
    }

    string_t TokenToString(const Token& tok)
    {
        // Delimiters as the environment sets them
        switch (tok.type)
        {
        case Token::ExprBegin:
            return m_delims.varBegin;
        case Token::ExprEnd:
            return m_delims.varEnd;
        case Token::StmtBegin:
            return m_delims.blockBegin;
        case Token::StmtEnd:
            return m_delims.blockEnd;
        case Token::CommentBegin:
            return m_delims.commentBegin;
        case Token::CommentEnd:
            return m_delims.commentEnd;
        default:
            break;
        }
        auto p = traits_t::s_tokens.find(tok.type);
        if (p != traits_t::s_tokens.end())
        {
            return p->second.template GetValueStr<CharT>();
        }

        if (tok.range.size() != 0)
        {
            return string_t(m_template->substr(tok.range.startOffset, tok.range.size()));
        }
        if (tok.type == Token::Identifier)
        {
            if (!tok.value.IsUndefined())
            {
                std::basic_string<CharT> tpl;
                return GetAsSameString(tpl, tok.value).value_or(std::basic_string<CharT>());
            }

            return UNIVERSAL_STR("<<Identifier>>").template GetValueStr<CharT>();
        }
        if (tok.type == Token::String)
        {
            return UNIVERSAL_STR("<<String>>").template GetValueStr<CharT>();
        }

        return string_t();
    }

    void OffsetToLinePos(size_t offset, unsigned& line, unsigned& col)
    {
        auto p = std::find_if(
            m_lines.begin(), m_lines.end(), [offset](const LineInfo& info) { return offset >= info.range.startOffset && offset < info.range.endOffset; });

        if (p == m_lines.end())
        {
            if (m_lines.empty() || offset != m_lines.back().range.endOffset)
            {
                line = 1;
                col = 1;
                return;
            }
            p = m_lines.end() - 1;
        }

        line = p->lineNumber + 1;
        col = static_cast<unsigned>(offset - p->range.startOffset + 1);
    }

    // The source line with a marker under the error column. A line longer than maxShownLen (a
    // minified template) is cut to a window around the column, so the message stays bounded.
    string_t GetLocationDescr(unsigned line, unsigned col)
    {
        if (line == 0 && col == 0)
        {
            return string_t();
        }

        --line;
        --col;

        static constexpr std::size_t maxShownLen = 160;
        static constexpr std::size_t windowHead = 40;
        static constexpr std::size_t windowLen = 120;
        static constexpr std::size_t headLen = 3;
        static constexpr std::size_t tailLen = 7;
        const CharT ellipsis[] = { static_cast<CharT>('.'), static_cast<CharT>('.'), static_cast<CharT>('.') };

        const auto& lineInfo = m_lines[line];
        std::basic_string_view<CharT> origLine(m_template->data() + lineInfo.range.startOffset, lineInfo.range.size());

        string_t result;
        std::size_t caretCol = col;
        if (origLine.size() > maxShownLen)
        {
            const std::size_t start = col > windowHead ? std::min<std::size_t>(col - windowHead, origLine.size() - windowLen) : 0;
            const std::size_t end = start + windowLen;
            result.reserve(windowLen + (2 * std::size(ellipsis)) + 1 + windowHead + headLen + 1 + tailLen + std::size(ellipsis));
            if (start != 0)
            {
                result.append(ellipsis, std::size(ellipsis));
            }
            caretCol = col - start + result.size();
            result.append(origLine.data() + start, windowLen);
            if (end != origLine.size())
            {
                result.append(ellipsis, std::size(ellipsis));
            }
        }
        else
        {
            result.reserve((origLine.size() * 2) + headLen + 1 + tailLen + 1);
            result.append(origLine.data(), origLine.size());
        }
        const std::size_t shownLen = result.size();
        result.append(1, static_cast<CharT>('\n'));

        // Leading whitespace is copied as is so that tabs keep the marker aligned
        auto locale = std::locale();
        std::size_t spacePrefixLen = 0;
        while (spacePrefixLen < shownLen && std::isspace(result[spacePrefixLen], locale))
        {
            ++spacePrefixLen;
        }

        if (caretCol < spacePrefixLen)
        {
            result.append(caretCol, static_cast<CharT>(' '));
        }
        else
        {
            const string_t spacePrefix = result.substr(0, spacePrefixLen);
            result.append(spacePrefix);
            const std::size_t actualHeadLen = std::min(caretCol - spacePrefixLen, headLen);
            result.append(caretCol - actualHeadLen - spacePrefixLen, static_cast<CharT>(' '));
            result.append(actualHeadLen, static_cast<CharT>('-'));
        }
        result.append(1, static_cast<CharT>('^'));
        result.append(tailLen, static_cast<CharT>('-'));

        return result;
    }

public:
    // LexerHelper interface
    std::string GetAsString(const CharRange& range) override { return traits_t::GetAsString(*m_template, range); }
    InternalValue GetAsValue(const CharRange& range, Token::Type type) override
    {
        if (type == Token::String)
        {
            // Jinja2 applies newline_sequence to the literal newlines of a string before decoding its escapes
            auto rawValue = CompileEscapes(ApplyNewlineSequence(m_template->data() + range.startOffset, range.size()));
            return InternalValue(TargetString(std::move(rawValue)));
        }
        if (type == Token::IntegerNum || type == Token::FloatNum)
        {
            return traits_t::RangeToNum(*m_template, range, type);
        }
        return InternalValue();
    }
    Keyword GetKeyword(const CharRange& range) override
    {
        return traits_t::FindKeyword(std::basic_string_view<CharT>(m_template->data() + range.startOffset, range.size()));
    }
    char GetCharAt(size_t /*pos*/) override { return '\0'; }

private:
    const string_t* m_template;
    const std::string& m_templateName;
    const Settings& m_settings;
    TemplateEnv* m_env = nullptr;
    NodeArena& m_nodes;
    const Delimiters& m_delims;
    // Inline room for the lines of small templates
    boost::container::small_vector<LineInfo, 8, void, boost::container::small_vector_options_t<boost::container::growth_factor<boost::container::growth_factor_100>>> m_lines;
    std::vector<TextBlockInfo> m_textBlocks;
    StatementInfoList* m_openStatements = nullptr;
    NameResolver m_names;
    TextBlockInfo m_currentBlockInfo = {};
    bool m_hasMetaBlock = false;
    mutable bool m_unbalancedBrackets = false;
    mutable bool m_unclosedString[2] = { false, false };
    std::basic_string_view<CharT> m_metadata;
    std::string m_metadataType;
    SourceLocation m_metadataLocation;
    std::unique_ptr<LexBuffers> m_lexBuffers;
    TemplateRootInfo m_root;
    std::unique_ptr<ConvertedTexts> m_convertedTexts;
};

template<typename T>
// NOLINTNEXTLINE(bugprone-throwing-static-initialization): only allocation can throw here, at load time
KeywordsInfo ParserTraitsBase<T>::s_keywordsInfo[43] = {
    { UNIVERSAL_STR("for"), Keyword::For },
    { UNIVERSAL_STR("endfor"), Keyword::Endfor },
    { UNIVERSAL_STR("in"), Keyword::In },
    { UNIVERSAL_STR("if"), Keyword::If },
    { UNIVERSAL_STR("else"), Keyword::Else },
    { UNIVERSAL_STR("elif"), Keyword::ElIf },
    { UNIVERSAL_STR("endif"), Keyword::EndIf },
    { UNIVERSAL_STR("or"), Keyword::LogicalOr },
    { UNIVERSAL_STR("and"), Keyword::LogicalAnd },
    { UNIVERSAL_STR("not"), Keyword::LogicalNot },
    { UNIVERSAL_STR("is"), Keyword::Is },
    { UNIVERSAL_STR("block"), Keyword::Block },
    { UNIVERSAL_STR("endblock"), Keyword::EndBlock },
    { UNIVERSAL_STR("extends"), Keyword::Extends },
    { UNIVERSAL_STR("macro"), Keyword::Macro },
    { UNIVERSAL_STR("endmacro"), Keyword::EndMacro },
    { UNIVERSAL_STR("call"), Keyword::Call },
    { UNIVERSAL_STR("endcall"), Keyword::EndCall },
    { UNIVERSAL_STR("filter"), Keyword::Filter },
    { UNIVERSAL_STR("endfilter"), Keyword::EndFilter },
    { UNIVERSAL_STR("autoescape"), Keyword::Autoescape },
    { UNIVERSAL_STR("endautoescape"), Keyword::EndAutoescape },
    { UNIVERSAL_STR("set"), Keyword::Set },
    { UNIVERSAL_STR("endset"), Keyword::EndSet },
    { UNIVERSAL_STR("include"), Keyword::Include },
    { UNIVERSAL_STR("import"), Keyword::Import },
    { UNIVERSAL_STR("true"), Keyword::True },
    { UNIVERSAL_STR("false"), Keyword::False },
    { UNIVERSAL_STR("True"), Keyword::True },
    { UNIVERSAL_STR("False"), Keyword::False },
    { UNIVERSAL_STR("none"), Keyword::None },
    { UNIVERSAL_STR("None"), Keyword::None },
    { UNIVERSAL_STR("recursive"), Keyword::Recursive },
    { UNIVERSAL_STR("scoped"), Keyword::Scoped },
    { UNIVERSAL_STR("with"), Keyword::With },
    { UNIVERSAL_STR("endwith"), Keyword::EndWith },
    { UNIVERSAL_STR("without"), Keyword::Without },
    { UNIVERSAL_STR("ignore"), Keyword::Ignore },
    { UNIVERSAL_STR("missing"), Keyword::Missing },
    { UNIVERSAL_STR("context"), Keyword::Context },
    { UNIVERSAL_STR("from"), Keyword::From },
    { UNIVERSAL_STR("as"), Keyword::As },
    { UNIVERSAL_STR("do"), Keyword::Do },
};

template<typename T>
// NOLINTNEXTLINE(bugprone-throwing-static-initialization): only allocation can throw here, at load time
std::unordered_map<int, MultiStringLiteral> ParserTraitsBase<T>::s_tokens = {
    { Token::Unknown, UNIVERSAL_STR("<<Unknown>>") },
    { Token::Lt, UNIVERSAL_STR("<") },
    { Token::Gt, UNIVERSAL_STR(">") },
    { Token::Plus, UNIVERSAL_STR("+") },
    { Token::Minus, UNIVERSAL_STR("-") },
    { Token::Percent, UNIVERSAL_STR("%") },
    { Token::Mul, UNIVERSAL_STR("*") },
    { Token::Div, UNIVERSAL_STR("/") },
    { Token::LBracket, UNIVERSAL_STR("(") },
    { Token::RBracket, UNIVERSAL_STR(")") },
    { Token::LSqBracket, UNIVERSAL_STR("[") },
    { Token::RSqBracket, UNIVERSAL_STR("]") },
    { Token::LCrlBracket, UNIVERSAL_STR("{") },
    { Token::RCrlBracket, UNIVERSAL_STR("}") },
    { Token::Assign, UNIVERSAL_STR("=") },
    { Token::Comma, UNIVERSAL_STR(",") },
    { Token::Colon, UNIVERSAL_STR(":") },
    { Token::Eof, UNIVERSAL_STR("<<End of block>>") },
    { Token::Equal, UNIVERSAL_STR("==") },
    { Token::NotEqual, UNIVERSAL_STR("!=") },
    { Token::LessEqual, UNIVERSAL_STR("<=") },
    { Token::GreaterEqual, UNIVERSAL_STR(">=") },
    { Token::StarStar, UNIVERSAL_STR("**") },
    { Token::DashDash, UNIVERSAL_STR("//") },
    { Token::LogicalOr, UNIVERSAL_STR("or") },
    { Token::LogicalAnd, UNIVERSAL_STR("and") },
    { Token::LogicalNot, UNIVERSAL_STR("not") },
    { Token::MulMul, UNIVERSAL_STR("**") },
    { Token::DivDiv, UNIVERSAL_STR("//") },
    { Token::True, UNIVERSAL_STR("true") },
    { Token::False, UNIVERSAL_STR("false") },
    { Token::None, UNIVERSAL_STR("none") },
    { Token::In, UNIVERSAL_STR("in") },
    { Token::Is, UNIVERSAL_STR("is") },
    { Token::For, UNIVERSAL_STR("for") },
    { Token::Endfor, UNIVERSAL_STR("endfor") },
    { Token::If, UNIVERSAL_STR("if") },
    { Token::Else, UNIVERSAL_STR("else") },
    { Token::ElIf, UNIVERSAL_STR("elif") },
    { Token::EndIf, UNIVERSAL_STR("endif") },
    { Token::Block, UNIVERSAL_STR("block") },
    { Token::EndBlock, UNIVERSAL_STR("endblock") },
    { Token::Extends, UNIVERSAL_STR("extends") },
    { Token::Macro, UNIVERSAL_STR("macro") },
    { Token::EndMacro, UNIVERSAL_STR("endmacro") },
    { Token::Call, UNIVERSAL_STR("call") },
    { Token::EndCall, UNIVERSAL_STR("endcall") },
    { Token::Filter, UNIVERSAL_STR("filter") },
    { Token::EndFilter, UNIVERSAL_STR("endfilter") },
    { Token::Autoescape, UNIVERSAL_STR("autoescape") },
    { Token::EndAutoescape, UNIVERSAL_STR("endautoescape") },
    { Token::Set, UNIVERSAL_STR("set") },
    { Token::EndSet, UNIVERSAL_STR("endset") },
    { Token::Include, UNIVERSAL_STR("include") },
    { Token::Import, UNIVERSAL_STR("import") },
    { Token::Recursive, UNIVERSAL_STR("recursive") },
    { Token::Scoped, UNIVERSAL_STR("scoped") },
    { Token::With, UNIVERSAL_STR("with") },
    { Token::EndWith, UNIVERSAL_STR("endwith") },
    { Token::Without, UNIVERSAL_STR("without") },
    { Token::Ignore, UNIVERSAL_STR("ignore") },
    { Token::Missing, UNIVERSAL_STR("missing") },
    { Token::Context, UNIVERSAL_STR("context") },
    { Token::From, UNIVERSAL_STR("form") },
    { Token::As, UNIVERSAL_STR("as") },
    { Token::Do, UNIVERSAL_STR("do") },
    { Token::RawBegin, UNIVERSAL_STR("{% raw %}") },
    { Token::RawEnd, UNIVERSAL_STR("{% endraw %}") },
    { Token::MetaBegin, UNIVERSAL_STR("{% meta %}") },
    { Token::MetaEnd, UNIVERSAL_STR("{% endmeta %}") },
    { Token::CommentBegin, UNIVERSAL_STR("{#") },
    { Token::CommentEnd, UNIVERSAL_STR("#}") },
    { Token::StmtBegin, UNIVERSAL_STR("{%") },
    { Token::StmtEnd, UNIVERSAL_STR("%}") },
    { Token::ExprBegin, UNIVERSAL_STR("{{") },
    { Token::ExprEnd, UNIVERSAL_STR("}}") },
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_PARSER_H
