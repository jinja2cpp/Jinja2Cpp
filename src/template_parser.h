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
#include "render_failure.h"
#include "renderer.h"
#include "small_vector.h"
#include "statements.h"
#include "unicode_tables.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <boost/algorithm/string/classification.hpp>
#include <nonstd/expected.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <forward_list>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
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
    // (which also cost a regex compilation per Load). Inlined into the one caller, which GCC
    // stopped doing once the parser grew (docs/tasks/0286)
    template<typename CharT>
    JINJA2CPP_ALWAYS_INLINE static Keyword FindKeyword(std::basic_string_view<CharT> name)
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
        // Keywords are short: a compare call per candidate would cost more than the compare
        auto entry = std::find_if(table.entries.begin() + table.runs[first], end, [name](const Entry& candidate) {
            if (candidate.name.size() != name.size())
            {
                return false;
            }
            for (std::size_t idx = 1; idx < name.size(); ++idx)
            {
                if (candidate.name[idx] != name[idx])
                {
                    return false;
                }
            }
            return true;
        });
        return entry == end ? Keyword::Unknown : entry->type;
    }
};

template<>
struct ParserTraits<char> : public ParserTraitsBase<>
{
    static std::string GetAsString(const std::string& str, CharRange range) { return str.substr(range.startOffset, range.size()); }
};

template<>
struct ParserTraits<wchar_t> : public ParserTraitsBase<>
{
    static std::string GetAsString(const std::wstring& str, CharRange range)
    {
        auto srcStr = str.substr(range.startOffset, range.size());
        return detail::StringConverter<std::wstring, std::string>::DoConvert(srcStr);
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

// The children of the bodies of the open statements, and where the tag of each starts in the
// source, for the locations of render errors (ComposedRenderer). Only the innermost open
// statement adds to its body, and a statement's body is made before its parent's goes on, so
// each body is the run of children from where its statement opened to the end: one list for all
// of them, not one in each statement on the stack
class StatementBodies // NOLINT(cppcoreguidelines-pro-type-member-init): m_inPlace is raw storage
{
public:
    using Child = NodeRef<IRendererBase>;

    StatementBodies() = default;
    StatementBodies(const StatementBodies&) = delete;
    StatementBodies& operator=(const StatementBodies&) = delete;
    ~StatementBodies() = default;

    // Where the body of a statement opened now starts
    [[nodiscard]] std::uint32_t Size() const { return m_size; }
    // To the innermost open body. Inline: a call costs more than the add
    JINJA2CPP_HOT_INLINE void Add(Child node, std::size_t offset)
    {
        if (m_size == m_capacity)
        {
            Grow();
        }
        new (m_data + m_size) Item{ node, ToOffset(offset) };
        ++m_size;
    }
    // At the start of the body that starts at `start`: the binding a frame starts with
    void AddFirst(std::uint32_t start, Child node, std::size_t offset)
    {
        Add(node, offset);
        std::rotate(m_data + start, m_data + m_size - 1, m_data + m_size);
    }
    // The body that starts at `start`, as a node; its children leave the list
    JINJA2CPP_HOT_INLINE NodeRef<ComposedRenderer> Take(NodeArena& nodes, std::uint32_t start)
    {
        assert(start <= m_size);
        const std::size_t count = m_size - start;
        const Item* const items = m_data + start;
        auto body = nodes.MakeWithItems<ComposedRenderer, Child, std::uint32_t>(
            count,
            [items, count](Child* children, std::uint32_t* offsets) {
                for (std::size_t idx = 0; idx != count; ++idx)
                {
                    children[idx] = items[idx].node;
                    // Last child first (ComposedRenderer::OffsetOf)
                    offsets[count - 1 - idx] = items[idx].offset;
                }
            },
            true);
        m_size = start;
        return body;
    }

private:
    struct Item
    {
        Child node;
        std::uint32_t offset = 0;
    };
    static_assert(std::is_trivially_copyable_v<Item> && std::is_trivially_destructible_v<Item>, "the list never destroys its children");

    // A source past 4 GiB locates its errors at the last offset that fits
    static std::uint32_t ToOffset(std::size_t offset) { return static_cast<std::uint32_t>(std::min<std::size_t>(offset, std::numeric_limits<std::uint32_t>::max())); }
    // Out of line: few templates have more children open at once than fit in place
    JINJA2CPP_COLD_INLINE void Grow()
    {
        if (m_capacity > std::numeric_limits<std::uint32_t>::max() / 2)
        {
            throw std::length_error("a template body too long");
        }
        const std::uint32_t capacity = m_capacity * 2;
        auto heap = std::make_unique<Item[]>(capacity);
        std::copy(m_data, m_data + m_size, heap.get());
        m_heap = std::move(heap);
        m_data = m_heap.get();
        m_capacity = capacity;
    }

    static constexpr std::uint32_t InPlace = 32;
    // Raw bytes, so that making the parser constructs no children: only those added are read
    alignas(Item) std::byte m_inPlace[InPlace * sizeof(Item)]; // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    std::unique_ptr<Item[]> m_heap;                            // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    Item* m_data = reinterpret_cast<Item*>(m_inPlace);
    std::uint32_t m_size = 0;
    std::uint32_t m_capacity = InPlace;
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
    // Where its body starts in the parser's StatementBodies; made into a ComposedRenderer when
    // the statement ends
    std::uint32_t bodyStart = 0;
    Token token;
    // Where its opening tag starts in the source
    std::size_t tagOffset = 0;
    NodeRef<IRendererBase> renderer;
    // A block's: what the enclosing body did with `self` before it (NodeArena::SelfUse). In the
    // padding after `renderer`, so the struct keeps its size
    NodeArena::SelfUse outerSelfUse = NodeArena::SelfUse::None;
    // Set on `{% trans %}` only
    std::shared_ptr<TransInfo> trans;
    // The resolver's frame of the body (0117 P1)
    NameResolver::FrameId frame = NameResolver::NoFrame;
    // A macro's or call block's: the tree's marks before its parameters, less what the tags
    // between them and its body made (MacroStatement::SetBodyMarks)
    NodeArena::ContextMarks marks;

    // Whether the body of a statement of `type` is a frame of the resolver of its own
    static bool OwnsFrame(Type type)
    {
        // One test of a mask, not a chain of compares: it runs for every statement (docs/tasks/0205)
        constexpr unsigned mask = (1U << TemplateRoot) | (1U << ForStatement) | (1U << BlockStatement) | (1U << MacroStatement) | (1U << MacroCallStatement) | (1U << WithStatement)
                                  | (1U << SetStatement) | (1U << FilterStatement) | (1U << AutoescapeStatement);
        return ((mask >> type) & 1U) != 0;
    }

    static StatementInfo Create(Type type, const Token& tok, std::size_t tagOffset)
    {
        StatementInfo result;
        result.type = type;
        result.token = tok;
        result.tagOffset = tagOffset;
        return result;
    }
};

// The statements open at the current tag, the template's root first. Few are open at once,
// so the stack stays inline; no reference to an entry is kept across a push
using StatementInfoList = SmallVector<StatementInfo, 4>;

// What the template's root learns from its tags: the blocks it defines and whether it
// extends another. The parse gives them to its TemplateRenderer at the end
struct TemplateRootInfo
{
    SmallVector<NodeRef<BlockStatement>, 4> blocks;
    bool hasExtends = false;
    // An `extends` at the top level, outside any `if`: Jinja2 compiles no output of the
    // template's body after it (has_known_extends)
    bool knownExtends = false;
    // A second `extends` after a known one: Jinja2 compiles nothing after it (CompilerExit)
    bool compilerExit = false;
    // A `{% autoescape %}` with a value that is not constant was parsed: only then can a tag
    // be in volatile code (TemplateParser::IsInVolatileCode)
    bool hasVolatileAutoescape = false;

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

// What becomes of an output tag (text, `{{ }}`, `trans`) of the template's body after an
// `extends`: Jinja2 compiles one in a macro, call block, set block or block; elsewhere it
// compiles none after a top-level `extends` and guards each after one in an `if`
// (require_output_check, docs/tasks/0203). Only a tag after an `extends` in the source can
// run after it: no loop holds one
enum class OutputCheck
{
    None,
    Drop,
    Guard
};
JINJA2CPP_OUTLINE_INLINE OutputCheck OutputCheckAfterExtends(const TemplateRootInfo& root, const StatementInfoList& stack)
{
    const bool compiled = std::any_of(stack.begin(), stack.end(), [](const StatementInfo& info) {
        return info.type == StatementInfo::MacroStatement || info.type == StatementInfo::MacroCallStatement || info.type == StatementInfo::SetStatement
               || info.type == StatementInfo::BlockStatement;
    });
    if (compiled)
    {
        return OutputCheck::None;
    }
    return root.knownExtends ? OutputCheck::Drop : OutputCheck::Guard;
}

// The text and expression tags test `hasExtends` themselves, so that the tags of a template
// with no `extends` keep a straight path at load
inline OutputCheck OutputCheckOf(const TemplateRootInfo& root, const StatementInfoList& stack)
{
    return root.hasExtends ? OutputCheckAfterExtends(root, stack) : OutputCheck::None;
}


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

    // A parser lives for one tag, which starts at `tagOffset` in the source; the settings and the arena outlive it
    StatementsParser(const Settings& settings, TemplateEnv* env, NodeArena& nodes, NameResolver& names, UnknownCallables& unknown, TemplateRootInfo& root, StatementBodies& bodies, std::size_t tagOffset)
        : m_settings(settings)
        , m_env(env)
        , m_nodes(nodes)
        , m_names(names)
        , m_unknown(unknown)
        , m_root(root)
        , m_bodies(bodies)
        , m_tagOffset(tagOffset)
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
    // A statement whose names Jinja2 visits before the expressions parsed among them (`with`
    // targets, macro parameters) stores `self` first when the body named it nowhere `before`
    void NoteStoresFirst(NodeArena::SelfUse before, bool storesSelf)
    {
        if (before == NodeArena::SelfUse::None && storesSelf)
        {
            m_nodes.ExchangeSelfUse(NodeArena::SelfUse::Store);
        }
    }
    ParseResult ParseAutoescape(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndAutoescape(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseInTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParsePluralize(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);
    ParseResult ParseEndTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok);

    // Opens `info`: its body starts with no children
    void Push(StatementInfoList& statementsInfo, StatementInfo&& info)
    {
        info.bodyStart = m_bodies.Size();
        statementsInfo.push_back(std::move(info));
    }
    // The statement on top of the stack, taken off it. Inlined: outlined, it costs every
    // closing tag a call and a move of the struct at load
    JINJA2CPP_HOT_INLINE static StatementInfo PopStatement(StatementInfoList& statementsInfo)
    {
        auto info = std::move(statementsInfo.back());
        statementsInfo.pop_back();
        return info;
    }
    // The statement's body, as a node
    NodeRef<ComposedRenderer> TakeBody(StatementInfo& info)
    {
        // The body of a frame starts by binding the names it holds from its entry (0168). Few
        // frames may bind one, and the rest stays inline (docs/tasks/0205)
        if (StatementInfo::OwnsFrame(info.type) && m_names.MayBindAtEntry(info.frame))
        {
            return TakeFrameBody(info);
        }
        return m_bodies.Take(m_nodes, info.bodyStart);
    }
    NodeRef<ComposedRenderer> TakeFrameBody(StatementInfo& info);
    // Whether the innermost open statement is a loop's `else`, whose body is a frame of its own
    static bool IsLoopElse(const StatementInfoList& statementsInfo)
    {
        const auto size = statementsInfo.size();
        return size > 1 && statementsInfo[size - 1].type == StatementInfo::ElseIfStatement && statementsInfo[size - 2].type == StatementInfo::ForStatement;
    }

    const Settings& m_settings;
    TemplateEnv* m_env;
    NodeArena& m_nodes;
    NameResolver& m_names;
    UnknownCallables& m_unknown;
    TemplateRootInfo& m_root;
    StatementBodies& m_bodies;
    // Where the tag starts in the source
    std::size_t m_tagOffset;
};

// `{{ name }}` inside `{% trans %}`: Jinja2 allows a plain name only. The result is the name
class TransVariableParser
{
public:
    using ParseResult = nonstd::expected<std::string, ParseError>;

    TransVariableParser(const Settings&, TemplateEnv*, NodeArena&, NameResolver&, UnknownCallables&) {}

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

// The value of a string literal with escapes or newlines to convert. Defined in
// template_parser.cpp, out of the translation unit that compiles the parser and the renderer
// together: most literals have none
template<typename CharT>
InternalValue EscapedLiteralValue(std::basic_string_view<CharT> text, const std::string& newlineSequence, bool& malformed);
extern template InternalValue EscapedLiteralValue<char>(std::string_view text, const std::string& newlineSequence, bool& malformed);
extern template InternalValue EscapedLiteralValue<wchar_t>(std::wstring_view text, const std::string& newlineSequence, bool& malformed);

template<typename CharT>
class TemplateParser final : public LexerHelper
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
        , m_unknown(setts.settings.userCallablesAsFilters)
        , m_metadataType(setts.settings.defaultMetadataType)
        , m_autoescape(setts.settings.autoescape)
    {
    }
    // Whether the template escapes its output, when Settings::autoescapeSelector decides it
    void SetAutoescape(bool autoescape) { m_autoescape = autoescape; }
    ParseResult Parse()
    {
        // One pass over the source: the splitter hands each block to the fine parse as soon as it
        // closes, so the tree is built while the source is read (docs/tasks/0142)
        auto templateRenderer = m_nodes.Make<TemplateRenderer>();
        StatementInfoList statementsStack;
        // Made in place: moving a StatementInfo in costs more than the rest of a small Load
        auto& root = statementsStack.emplace_back();
        root.type = StatementInfo::TemplateRoot;
        root.frame = m_names.PushUnit(NameResolver::NoFrame, templateRenderer);
        m_openStatements = &statementsStack;
        auto roughResult = DoRoughParsing();
        m_openStatements = nullptr;

        // An error of the splitter is the only one reported, as if the blocks had not been parsed
        if (!roughResult)
        {
            return ParseErrorsToErrorInfo(roughResult.error());
        }

        auto fineResult = FinishFineParsing(templateRenderer, statementsStack);
        if (!fineResult)
        {
            return ParseErrorsToErrorInfo(fineResult.error());
        }
        // Jinja2 checks the names of filters and tests when it compiles, after a parse without errors
        if (const auto* unknown = m_unknown.First())
        {
            return ParseErrorsToErrorInfo(std::vector<ParseError>{ ParseError(unknown->isTest ? ErrorCode::UnknownTest : ErrorCode::UnknownFilter, unknown->token) });
        }
        m_names.Resolve(m_nodes);

        return templateRenderer;
    }

    // Frees what only the parse needed. Load calls it before Seal makes the tree's buffer,
    // so that the two are not held at once
    void ReleaseParseState()
    {
        m_lexBuffers.reset();
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
        // Where the tag starts, at its begin delimiter; where the text starts for text
        std::size_t tagStart = 0;
        TextBlockType type;
        // The tokens of the tag are in m_lexBuffers already: the splitter lexed it to find its end
        bool preLexed = false;
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

    // The lines of the source, for the positions of errors and of the metadata: found when one
    // is needed
    void SplitLines()
    {
        if (!m_lines.empty())
        {
            return;
        }
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
    RoughMatch FindNextMatch(size_t pos)
    {
        switch (m_currentBlockInfo.type)
        {
        case TextBlockType::RawText:
            return FindTagInText(pos);
        case TextBlockType::Expr:
            return LexToBlockEnd(pos, m_delims.varEnd, RM_ExprEnd);
        case TextBlockType::Statement:
            return LexToBlockEnd(pos, m_delims.blockEnd, RM_StmtEnd);
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

    // The end of an expression or a statement, found by lexing the tag, whose tokens the fine
    // parse then takes. Where the lexer cannot tell (see TagLexer::LexToEnd), the end comes from
    // FindBlockEnd, and the fine parse lexes the tag again on its own.
    RoughMatch LexToBlockEnd(size_t pos, const string_t& end, unsigned type)
    {
        const bool usable = type == RM_ExprEnd ? m_varEndLexable : m_blockEndLexable;
        if (usable && !m_unbalancedBrackets && !m_unclosedString[0] && !m_unclosedString[1])
        {
            if (!m_lexBuffers)
            {
                m_lexBuffers = std::make_unique<LexBuffers>();
            }
            TagLexer<CharT, TemplateParser> lexer(*m_template, *this, m_lexBuffers->tokens);
            const auto endPos = lexer.LexToEnd(m_currentBlockInfo.range.startOffset, end, type != RM_ExprEnd);
            if (endPos != string_t::npos)
            {
                m_currentBlockInfo.preLexed = true;
                return MakeMatch(type, endPos, end.size());
            }
        }
        return FindBlockEnd(pos, end, type);
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
        m_currentBlockInfo.tagStart = matchStart;

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
        // Jinja2 has no `+}}`: before the end of an expression a `+` is the operator
        auto ctrlChar = endPos > m_currentBlockInfo.range.startOffset ? (*m_template)[endPos - 1] : CharT();
        if (ctrlChar == '-' || (ctrlChar == '+' && m_currentBlockInfo.type != TextBlockType::Expr))
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
        FineParseBlock(m_currentBlockInfo);
        m_currentBlockInfo.type = TextBlockType::RawText;
        m_currentBlockInfo.preLexed = false;
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
            while (const auto len = SpaceLengthAt(newPos))
            {
                newPos += len;
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

        auto& tpl = *m_template;
        auto originalOffset = endOffset;
        bool sameLine = true;
        for (size_t len = 0; endOffset != currentBlockInfo.range.startOffset && endOffset > 0; endOffset -= len)
        {
            auto ch = tpl[endOffset - 1];
            len = SpaceLengthBefore(endOffset, currentBlockInfo.range.startOffset);
            if (!len)
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

    // The length of the whitespace character at `pos` (Python's \s, Unicode included), or 0
    [[nodiscard]] size_t SpaceLengthAt(size_t pos) const
    {
        const auto& tpl = *m_template;
        if (pos >= tpl.size())
        {
            return 0;
        }
        const auto unit = static_cast<std::make_unsigned_t<CharT>>(tpl[pos]);
        if constexpr (sizeof(CharT) == 1)
        {
            if (unit >= 0x80)
            {
                const size_t len = unit >= 0xF0 ? 4 : (unit >= 0xE0 ? 3 : 2);
                return pos + len <= tpl.size() && unicode::IsSpace(DecodeUtf8(pos, len)) ? len : 0;
            }
        }
        return unicode::IsSpace(unit) ? 1 : 0;
    }

    // The length of the whitespace character that ends at `pos`, not reaching back before `begin`, or 0
    [[nodiscard]] size_t SpaceLengthBefore(size_t pos, size_t begin) const
    {
        const auto& tpl = *m_template;
        const auto unit = static_cast<std::make_unsigned_t<CharT>>(tpl[pos - 1]);
        if constexpr (sizeof(CharT) == 1)
        {
            if (unit >= 0x80)
            {
                // Back over the continuation bytes to the lead byte
                size_t start = pos - 1;
                while (start != begin && pos - start < 4 && (static_cast<unsigned char>(tpl[start]) & 0xC0) == 0x80)
                {
                    --start;
                }
                return SpaceLengthAt(start) == pos - start ? pos - start : 0;
            }
        }
        return unicode::IsSpace(unit) ? 1 : 0;
    }

    // The code point of the UTF-8 sequence of `len` bytes at `pos`, or 0 (no space) when the
    // sequence is not well formed
    [[nodiscard]] uint32_t DecodeUtf8(size_t pos, size_t len) const
    {
        const auto& tpl = *m_template;
        if (static_cast<unsigned char>(tpl[pos]) < 0xC2)
        {
            return 0;
        }
        uint32_t cp = static_cast<unsigned char>(tpl[pos]) & (0x7F >> len);
        for (size_t idx = 1; idx != len; ++idx)
        {
            const auto unit = static_cast<unsigned char>(tpl[pos + idx]);
            if ((unit & 0xC0) != 0x80)
            {
                return 0;
            }
            cp = (cp << 6) | (unit & 0x3F);
        }
        // Overlong forms: three bytes start at U+0800, four at U+10000
        return cp >= (len == 3 ? 0x800U : (len == 4 ? 0x10000U : 0x80U)) ? cp : 0;
    }

    // Out of line: rarely needed, and inlined into its one caller it would keep MakeRawTextRenderer
    // out of FineParseBlock
    JINJA2CPP_NOINLINE_INLINE string_t ApplyNewlineSequence(const CharT* text, size_t size) const
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

    // Parses a block the splitter has closed; after an error that stops the parse, only the
    // splitter goes on, which may still find an error of its own to report
    void FineParseBlock(const TextBlockInfo& block)
    {
        if (m_fineParseStopped)
        {
            return;
        }
        auto& statementsStack = *m_openStatements;
        auto& errors = m_fineErrors;
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
            m_fineParseStopped = !FineParseStatement(block, statementsStack, errors);
            break;
        default:
            break;
        }
    }

    nonstd::expected<void, std::vector<ParseError>> FinishFineParsing(NodeRef<TemplateRenderer> templateRef, StatementInfoList& statementsStack)
    {
        auto errors = std::move(m_fineErrors);
        if (m_fineParseStopped)
        {
            return MakeUnexpected(std::move(errors));
        }

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

        if (const auto binding = m_names.EntryBinding(statementsStack.front().frame, m_nodes))
        {
            m_bodies.AddFirst(0, binding, 0);
        }
        auto& templateRoot = m_nodes[templateRef];
        templateRoot.SetBlocks(m_nodes.MakeSpan(m_root.blocks));
        templateRoot.SetAutoescape(m_autoescape);
        for (const auto block : m_root.blocks)
        {
            m_nodes[block].SetAutoescape(m_autoescape);
        }
        if (m_root.hasExtends)
        {
            templateRoot.SetHasExtends();
        }
        // A parent it extends may read it, which an include learns only once `extends` has run
        // (docs/tasks/0298)
        if (m_nodes.ReadsOuterLoop() || m_root.hasExtends)
        {
            templateRoot.SetMentionsLoop();
        }
        if (m_nodes.FirstSelfUse() == NodeArena::SelfUse::Store)
        {
            templateRoot.SetSelfFromContext();
        }
        templateRoot.SetBody(m_bodies.Take(m_nodes, 0));
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
        auto check = OutputCheck::None;
        if (m_root.hasExtends)
        {
            check = OutputCheckAfterExtends(m_root, statementsStack);
            if (check == OutputCheck::Drop)
            {
                return;
            }
        }
        // One call: a second makes the compiler outline the function
        NodeRef<IRendererBase> text = MakeRawTextRenderer(range);
        if (check == OutputCheck::Guard)
        {
            text = m_nodes.Make<OutputCheckStatement>(text);
        }
        m_bodies.Add(text, range.startOffset);
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
        const auto unknownMark = m_unknown.Mark();
        auto parseResult = InvokeParser<NodeRef<IRendererBase>, ExpressionParser>(block, IsInVolatileCode(statementsStack));
        if (m_unknown.Mark() != unknownMark)
        {
            PlaceUnknownCallables(statementsStack, statementsStack.size(), unknownMark, true);
        }
        if (parseResult)
        {
            // Parsed all the same: Jinja2 reports what does not parse after `extends` too
            const auto check = m_root.hasExtends ? OutputCheckAfterExtends(m_root, statementsStack) : OutputCheck::None;
            if (check == OutputCheck::None)
            {
                m_bodies.Add(*parseResult, block.tagStart);
            }
            else if (check == OutputCheck::Guard)
            {
                m_bodies.Add(m_nodes.Make<OutputCheckStatement>(*parseResult), block.tagStart);
            }
        }
        else
        {
            errors.push_back(parseResult.error());
        }
    }

    // False when parsing has to stop here
    bool FineParseStatement(const TextBlockInfo& block, StatementInfoList& statementsStack, std::vector<ParseError>& errors)
    {
        const auto unknownMark = m_unknown.Mark();
        const auto depth = statementsStack.size();
        auto parseResult = InvokeParser<void, StatementsParser>(block, statementsStack);
        if (m_unknown.Mark() != unknownMark)
        {
            // A statement opens its body after its expressions: they are where it starts
            const bool isTrans = statementsStack.size() > depth && statementsStack.back().type == StatementInfo::TransStatement;
            PlaceUnknownCallables(statementsStack, std::min(depth, statementsStack.size()), unknownMark, isTrans);
        }
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

    // Places the unknown names a tag added since `mark`, inside the first `depth` statements of the
    // stack: Jinja2 defers the check in an `if` statement, its tests and branches, up to the next
    // statement that makes a frame of its own (a loop and its `else`, a macro, a call block, a
    // block, `with`, a filter or set block, `autoescape`). An output tag (`{{ }}`, `trans`) after
    // a known `extends` is not compiled at all, unless it is in a macro, call block, set block or
    // block, whose output Jinja2 compiles
    void PlaceUnknownCallables(const StatementInfoList& statementsStack, std::size_t depth, std::size_t mark, bool isOutput)
    {
        bool deferred = false;
        bool placed = false;
        bool compiled = !isOutput || !m_root.knownExtends;
        std::size_t unit = 0;
        const auto end = statementsStack.rend();
        for (auto p = std::prev(end, static_cast<std::ptrdiff_t>(depth)); p != end; ++p)
        {
            switch (p->type)
            {
            case StatementInfo::MacroStatement:
            case StatementInfo::MacroCallStatement:
            case StatementInfo::SetStatement:
            case StatementInfo::BlockStatement:
                compiled = true;
                break;
            default:
                break;
            }
            if (!placed)
            {
                switch (p->type)
                {
                case StatementInfo::IfStatement:
                    deferred = true;
                    placed = true;
                    break;
                case StatementInfo::ElseIfStatement:
                {
                    // An `elif` or `else` of an `if`, or the `else` of a loop
                    auto head = p;
                    while (head->type == StatementInfo::ElseIfStatement && std::next(head) != end)
                    {
                        ++head;
                    }
                    deferred = head->type == StatementInfo::IfStatement;
                    placed = true;
                    break;
                }
                case StatementInfo::ExtendsStatement:
                case StatementInfo::TransStatement:
                    break;
                default:
                    placed = true;
                    break;
                }
            }
            if (p->type == StatementInfo::BlockStatement)
            {
                unit = p->token.range.startOffset + 1;
                break;
            }
        }
        if (!compiled || m_root.compilerExit)
        {
            m_unknown.DeferSince(mark);
            return;
        }
        m_unknown.Place(mark, deferred, unit);
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

    // Whether the innermost tag is in volatile code: inside `{% autoescape %}` with a value that
    // is not constant (RenderContext::IsVolatileAutoescape). A block body is not, as Jinja2
    // compiles it on its own
    bool IsInVolatileCode(const StatementInfoList& statementsStack) const
    {
        if (!m_root.hasVolatileAutoescape)
        {
            return false;
        }
        for (auto info = statementsStack.rbegin(); info != statementsStack.rend() && info->type != StatementInfo::BlockStatement; ++info)
        {
            if (info->type == StatementInfo::AutoescapeStatement && m_nodes.Get<AutoescapeStatement>(info->renderer).IsVolatile())
            {
                return true;
            }
        }
        return false;
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
        // Room for the tokens of a typical tag, so the first tags do not grow the list one token at a time
        LexBuffers() { tokens.reserve(16); }

        TokensList tokens;
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
        auto& tokens = buffers.tokens;
        auto range = block.range;
        if (!block.preLexed && !TagLexer<CharT, TemplateParser>(*m_template, *this, tokens).LexRange(range.startOffset, range.endOffset))
        {
            return MakeParseError(ErrorCode::Unspecified, MakeToken(Token::Unknown, { range.startOffset, range.startOffset + 1 }));
        }

        MarkMacroSpecialNames(tokens, std::is_same_v<P, StatementsParser>);

        auto praser = MakeParser<P>(block);
        LexScanner scanner(tokens, this);
        auto result = praser.Parse(scanner, std::forward<Args>(args)...);
        if (!result)
        {
            return MakeUnexpected(result.error());
        }

        return result;
    }

    // A parser for one tag: a statement's adds the blocks it defines to the root's
    template<typename P>
    P MakeParser(const TextBlockInfo& block)
    {
        if constexpr (std::is_same_v<P, StatementsParser>)
        {
            return P(m_settings, m_env, m_nodes, m_names, m_unknown, m_root, m_bodies, block.tagStart);
        }
        else
        {
            return P(m_settings, m_env, m_nodes, m_names, m_unknown);
        }
    }

    // Tells the enclosing macros and call blocks which of `caller`, `varargs` and `kwargs`
    // their bodies use before assigning them. Jinja2 decides the same with find_undeclared,
    // which visits assignment targets and parameters before the values; so does this scan,
    // in source order, block by block.
    void MarkMacroSpecialNames(const TokensList& tokens, bool isStatement)
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

    bool IsCallerString(const Token& tok) const { return tok.type == Token::String && AsString(LiteralValue(tok.range, tok.type)) == "caller"; }

    static void MarkSetTargets(const TokensList& tokens, std::vector<bool>& isStore)
    {
        for (std::size_t idx = 1; idx < tokens.size() && tokens[idx] != Token::Assign && tokens[idx] != '|'; ++idx)
        {
            isStore[idx] = tokens[idx].type == Token::Identifier;
        }
    }

    static void MarkForTargets(const TokensList& tokens, std::vector<bool>& isStore)
    {
        for (std::size_t idx = 1; idx < tokens.size() && tokens[idx] != Token::In && tokens[idx].keyword != Keyword::In; ++idx)
        {
            isStore[idx] = tokens[idx].type == Token::Identifier;
        }
    }

    static void MarkWithTargets(const TokensList& tokens, std::vector<bool>& isStore)
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
    static void MarkMacroParamNames(const TokensList& tokens, std::size_t idx, std::vector<bool>& isStore)
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
    void CollectSpecialNameUses(const TokensList& tokens, const std::vector<bool>& isStore, unsigned& stores, unsigned& loads) const
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

    static Token MakeToken(Token::Type type, const CharRange& range)
    {
        Token tok;
        tok.type = type;
        tok.range = range;
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
        SplitLines();
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

    // The source line with a marker under the error column (detail::DescribeLocation)
    string_t GetLocationDescr(unsigned line, unsigned col)
    {
        if (line == 0 && col == 0)
        {
            return string_t();
        }
        const auto& lineInfo = m_lines[line - 1];
        return detail::DescribeLocation(std::basic_string_view<CharT>(m_template->data() + lineInfo.range.startOffset, lineInfo.range.size()), col - 1);
    }

public:
    // LexerHelper interface
    std::string GetAsString(const CharRange& range) override { return traits_t::GetAsString(*m_template, range); }
    [[nodiscard]] const char* NarrowSource() const override
    {
        if constexpr (std::is_same_v<CharT, char>)
        {
            return m_template->data();
        }
        else
        {
            return nullptr;
        }
    }
    std::string_view GetAsConvertedView(const CharRange& range) override
    {
        m_convertedName = GetAsString(range);
        return m_convertedName;
    }
    InternalValue GetAsValue(const CharRange& range, Token::Type type) override { return LiteralValue(range, type); }
    bool TakeLiteralError() override { return std::exchange(m_literalError, false); }
    Keyword GetKeyword(const CharRange& range) override
    {
        return traits_t::FindKeyword(std::basic_string_view<CharT>(m_template->data() + range.startOffset, range.size()));
    }

private:
    InternalValue LiteralValue(const CharRange& range, Token::Type type) const
    {
        if (type == Token::String)
        {
            const CharT* text = m_template->data() + range.startOffset;
            if (std::none_of(text, text + range.size(), [](CharT ch) { return ch == '\\' || ch == '\n'; }))
            {
                // Most literals have nothing to convert: copied straight into the value
                m_literalError = false;
                return InternalValue::FromText(std::basic_string_view<CharT>(text, range.size()));
            }
            return EscapedLiteralValue(std::basic_string_view<CharT>(text, range.size()), m_settings.newlineSequence, m_literalError);
        }
        if (type == Token::IntegerNum || type == Token::FloatNum)
        {
            return ParseNumberLiteral(traits_t::GetAsString(*m_template, range));
        }
        return InternalValue();
    }

    // Set by LiteralValue (const: it is a cache of the last literal), taken by TakeLiteralError
    mutable bool m_literalError = false;
    const string_t* m_template;
    const std::string& m_templateName;
    const Settings& m_settings;
    TemplateEnv* m_env = nullptr;
    NodeArena& m_nodes;
    const Delimiters& m_delims;
    // Inline room for the lines of small templates
    SmallVector<LineInfo, 8, SmallVectorDoubling> m_lines;
    StatementInfoList* m_openStatements = nullptr;
    // The children of the open statements' bodies
    StatementBodies m_bodies;
    // The errors of the fine parse so far, and whether one of them stopped it
    std::vector<ParseError> m_fineErrors;
    bool m_fineParseStopped = false;
    // Whether the lexer can find the end delimiters (TagLexer::UsableEnd)
    bool m_varEndLexable = TagLexer<CharT, TemplateParser>::UsableEnd(m_delims.varEnd);
    bool m_blockEndLexable = TagLexer<CharT, TemplateParser>::UsableEnd(m_delims.blockEnd);
    NameResolver m_names;
    UnknownCallables m_unknown;
    TextBlockInfo m_currentBlockInfo = {};
    bool m_hasMetaBlock = false;
    mutable bool m_unbalancedBrackets = false;
    mutable bool m_unclosedString[2] = { false, false };
    std::basic_string_view<CharT> m_metadata;
    std::string m_metadataType;
    SourceLocation m_metadataLocation;
    std::unique_ptr<LexBuffers> m_lexBuffers;
    TemplateRootInfo m_root;
    bool m_autoescape;
    std::unique_ptr<ConvertedTexts> m_convertedTexts;
    // The last name GetAsConvertedView converted
    std::string m_convertedName;
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

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_PARSER_H
