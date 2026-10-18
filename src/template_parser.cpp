#include "template_parser.h"

#include "error_handling.h"
#include "expression_evaluator.h"
#include "expression_parser.h"
#include "helpers.h"
#include "internal_value.h"
#include "lexer.h"
#include "make_unexpected.h"
#include "name_resolver.h"
#include "node_arena.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "renderer.h"
#include "statements.h"

#include <jinja2cpp/error_info.h>

#include <boost/container/small_vector.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{

NodeRef<ComposedRenderer> StatementsParser::TakeFrameBody(StatementInfo& info)
{
    if (const auto binding = m_names.EntryBinding(info.frame, m_nodes))
    {
        info.body.insert(info.body.begin(), binding);
    }
    auto body = m_nodes.MakeWithItems<ComposedRenderer>(boost::span<const ComposedRenderer::Child>(info.body.data(), info.body.size()));
    info.body.clear();
    return body;
}

StatementsParser::ParseResult StatementsParser::Parse(LexScanner& lexer, StatementInfoList& statementsInfo)
{
    const auto& tok = lexer.NextToken();
    ParseResult result;
    // The statement's expressions are read in the frame of the body it is in, and a body it
    // opens is in that frame too unless the statement makes one of its own
    const auto outerFrame = statementsInfo.empty() ? NameResolver::NoFrame : statementsInfo.back().frame;
    m_names.SetCurrent(outerFrame);
    // A statement that does not open a frame of its own puts the ones inside it in a branch;
    // a loop's `else` has one (docs/tasks/0202)
    m_names.SetInBranch(!statementsInfo.empty() && !StatementInfo::OwnsFrame(statementsInfo.back().type) && !IsLoopElse(statementsInfo));

    auto keyword = tok.keyword;
    // Jinja2: required blocks can only contain comments or whitespace
    if (keyword != Keyword::EndBlock && !statementsInfo.empty() && statementsInfo.back().type == StatementInfo::BlockStatement && m_nodes.Get<BlockStatement>(statementsInfo.back().renderer).IsRequired())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, tok);
    }

    // Jinja2's i18n extension: a trans block holds only text, `{{ name }}`, one pluralize and its endtrans
    if (!statementsInfo.empty() && statementsInfo.back().type == StatementInfo::TransStatement)
    {
        result = ParseInTrans(lexer, statementsInfo, tok);
        if (result && lexer.PeekNextToken() != Token::Eof)
        {
            return MakeParseError(ErrorCode::ExpectedEndOfStatement, lexer.PeekNextToken());
        }
        return result;
    }

    switch (keyword)
    {
    case Keyword::For:
        result = ParseFor(lexer, statementsInfo, tok);
        break;
    case Keyword::Endfor:
        result = ParseEndFor(lexer, statementsInfo, tok);
        break;
    case Keyword::If:
        result = ParseIf(lexer, statementsInfo, tok);
        break;
    case Keyword::Else:
        result = ParseElse(lexer, statementsInfo, tok);
        break;
    case Keyword::ElIf:
        result = ParseElIf(lexer, statementsInfo, tok);
        break;
    case Keyword::EndIf:
        result = ParseEndIf(lexer, statementsInfo, tok);
        break;
    case Keyword::Set:
        result = ParseSet(lexer, statementsInfo, tok);
        break;
    case Keyword::EndSet:
        result = ParseEndSet(lexer, statementsInfo, tok);
        break;
    case Keyword::Block:
        result = ParseBlock(lexer, statementsInfo, tok);
        break;
    case Keyword::EndBlock:
        result = ParseEndBlock(lexer, statementsInfo, tok);
        break;
    case Keyword::Extends:
        result = ParseExtends(lexer, statementsInfo, tok);
        break;
    case Keyword::Macro:
        result = ParseMacro(lexer, statementsInfo, tok);
        break;
    case Keyword::EndMacro:
        result = ParseEndMacro(lexer, statementsInfo, tok);
        break;
    case Keyword::Call:
        result = ParseCall(lexer, statementsInfo, tok);
        break;
    case Keyword::EndCall:
        result = ParseEndCall(lexer, statementsInfo, tok);
        break;
    case Keyword::Include:
        result = ParseInclude(lexer, statementsInfo, tok);
        break;
    case Keyword::Import:
        result = ParseImport(lexer, statementsInfo, tok);
        break;
    case Keyword::From:
        result = ParseFrom(lexer, statementsInfo, tok);
        break;
    case Keyword::Do:
        if (!m_settings.extensions.doStatement)
        {
            return MakeParseError(ErrorCode::ExtensionDisabled, tok);
        }
        result = ParseDo(lexer, statementsInfo, tok);
        break;
    case Keyword::With:
        result = ParseWith(lexer, statementsInfo, tok);
        break;
    case Keyword::EndWith:
        result = ParseEndWith(lexer, statementsInfo, tok);
        break;
    case Keyword::Filter:
        result = ParseFilter(lexer, statementsInfo, tok);
        break;
    case Keyword::EndFilter:
        result = ParseEndFilter(lexer, statementsInfo, tok);
        break;
    case Keyword::Autoescape:
        result = ParseAutoescape(lexer, statementsInfo, tok);
        break;
    case Keyword::EndAutoescape:
        result = ParseEndAutoescape(lexer, statementsInfo, tok);
        break;
    default:
        result = ParseNonKeywordStatement(lexer, statementsInfo, tok);
        break;
    }

    if (result && !statementsInfo.empty() && statementsInfo.back().frame == NameResolver::NoFrame)
    {
        statementsInfo.back().frame = outerFrame;
    }
    if (result)
    {
        // Each open block is a level of render recursion; elif and else branches are not
        const auto openBlocks = std::count_if(statementsInfo.begin(), statementsInfo.end(), [](const StatementInfo& info) {
            return info.type != StatementInfo::ElseIfStatement;
        });
        if (static_cast<std::size_t>(openBlocks) > LimitOrNone(m_settings.limits.maxBlockNesting))
        {
            return MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
        }
        const auto& next = lexer.PeekNextToken();
        if (next != Token::Eof)
        {
            return MakeParseError(ErrorCode::ExpectedEndOfStatement, next);
        }
    }

    return result;
}

// A statement whose name is not a keyword: `break`, `continue` and `trans` are extension statements
StatementsParser::ParseResult StatementsParser::ParseNonKeywordStatement(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& tok)
{
    // `break` and `continue` are not keywords in Jinja2: they stay usable as names
    if (tok == Token::Identifier && (lexer.GetAsString(tok) == "break" || lexer.GetAsString(tok) == "continue"))
    {
        if (!m_settings.extensions.loopControls)
        {
            return MakeParseError(ErrorCode::ExtensionDisabled, tok);
        }
        return ParseLoopControl(statementsInfo, tok, lexer.GetAsString(tok) == "break" ? LoopControl::Break : LoopControl::Continue);
    }
    if (m_settings.extensions.i18n && tok == Token::Identifier && lexer.GetAsString(tok) == "trans")
    {
        return ParseTrans(lexer, statementsInfo, tok);
    }
    return MakeParseError(ErrorCode::UnexpectedToken, tok);
}

struct ErrorTokenConverter
{
    const Token& baseTok;

    explicit ErrorTokenConverter(const Token& t)
        : baseTok(t)
    {}

    Token operator()(const Token& tok) const
    {
        return tok;
    }

    template<typename T>
    Token operator()(T tokType) const
    {
        auto newTok = baseTok;
        newTok.type = static_cast<Token::Type>(tokType);
        if (newTok.type == Token::Identifier || newTok.type == Token::String)
        {
            newTok.range.endOffset = newTok.range.startOffset;
        }
        return newTok;
    }
};

template<typename... Args>
auto MakeParseErrorTL(ErrorCode code, const Token& baseTok, const Args&... expectedTokens)
{
    ErrorTokenConverter tokCvt(baseTok);

    return MakeParseError(code, baseTok, { tokCvt(expectedTokens)... });
}

namespace
{
// The statement being parsed ends with `recursive` (Jinja2's `for x in l if c recursive`)
bool EndsWithRecursive(LexScanner& lexer)
{
    const auto state = lexer.GetState();
    Keyword last = Keyword::Unknown;
    for (const Token* tok = &lexer.NextToken(); *tok != Token::Eof; tok = &lexer.NextToken())
    {
        last = tok->keyword;
    }
    lexer.RestoreState(state);
    return last == Keyword::Recursive;
}

// The tokens from `from` up to the scanner's position have the identifier `name`, as a name
// or an attribute: enough to tell that an expression cannot read the name
bool ReadsName(LexScanner& lexer, const LexScanner::State& from, std::string_view name)
{
    const auto state = lexer.GetState();
    lexer.RestoreState(from);
    bool isRead = false;
    while (!isRead && lexer.GetState().m_cur != state.m_cur)
    {
        const auto& tok = lexer.NextToken();
        isRead = tok == Token::Identifier && lexer.GetAsView(tok) == name;
    }
    lexer.RestoreState(state);
    return isRead;
}
} // namespace

StatementsParser::ParseResult StatementsParser::ParseFor(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto targetTok = lexer.PeekNextToken();
    auto target = ParseAssignTarget(lexer, false);
    if (!target)
    {
        return MakeUnexpected(target.error());
    }
    // Jinja2: "Can't assign to special loop variable in for-loop target"
    if (target->namesLoop)
    {
        return MakeParseError(ErrorCode::UnexpectedToken, targetTok);
    }

    if (!lexer.EatIfEqual(Keyword::In))
    {
        const Token& tok1 = lexer.PeekNextToken();
        Token tok2 = tok1;
        tok2.type = Token::Identifier;
        tok2.range.endOffset = tok2.range.startOffset;
        return MakeParseErrorTL(ErrorCode::ExpectedToken, tok1, tok2, Token::In, ',');
    }

    ExpressionParser exprPraser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto valueExpr = exprPraser.ParseTupleOrExpression(lexer, false);
    if (!valueExpr)
    {
        return MakeUnexpected(valueExpr.error());
    }

    // Jinja2 takes `recursive` after the filter only: `for x in l if c recursive`
    bool isRecursive = false;
    const auto outerFrame = m_names.Current();
    auto filterFrame = NameResolver::NoFrame;
    NodeRef<Expression> ifExpr;
    bool filterReadsLoop = false;
    if (lexer.EatIfEqual(Keyword::If))
    {
        // The recursive flag after the filter is not known yet: the filter's frame is a
        // dynamic region either way when the loop is recursive, so look ahead for it
        const auto filterStart = lexer.GetState();
        filterFrame = m_names.PushFilter(outerFrame, EndsWithRecursive(lexer));
        m_names.SetCurrent(filterFrame);
        // A frame of its own to Jinja2, even in an `if`
        CheckGuard checked(m_unknown, UnknownCallables::Check::Now);
        auto parsedExpr = exprPraser.ParseFullExpression(lexer);
        m_names.SetCurrent(outerFrame);
        if (!parsedExpr)
        {
            return MakeUnexpected(parsedExpr.error());
        }
        ifExpr = *parsedExpr;
        filterReadsLoop = ReadsName(lexer, filterStart, "loop");
        isRecursive = lexer.EatIfEqual(Keyword::Recursive);
    }
    else if (lexer.EatIfEqual(Keyword::Recursive))
    {
        isRecursive = true;
    }

    if (!ifExpr && lexer.PeekNextToken() != Token::Eof)
    {
        const auto& tok1 = lexer.PeekNextToken();
        return MakeParseErrorTL(ErrorCode::ExpectedToken, tok1, Token::If, Token::Recursive, Token::Eof);
    }

    auto renderer = m_nodes.Make<ForStatement>(target->target, *valueExpr, ifExpr, isRecursive);
    m_nodes.Get<ForStatement>(renderer).SetFilterReadsLoop(filterReadsLoop);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ForStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementInfo.frame = m_names.PushFor(outerFrame, renderer, isRecursive);
    if (filterFrame != NameResolver::NoFrame)
    {
        m_names.LinkFilter(filterFrame, statementInfo.frame);
    }
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

namespace
{
// Jinja2's parse_assign_target, see StatementsParser::ParseAssignTarget. Writes the target's
// nodes in pre-order, its names' texts straight into the tree
class AssignTargetParser
{
public:
    using Result = nonstd::expected<void, ParseError>;

    AssignTargetParser(LexScanner& lexer, NodeArena& nodes, unsigned maxDepth)
        : m_lexer(lexer)
        , m_nodes(nodes)
        , m_maxDepth(maxDepth)
    {
    }

    nonstd::expected<ParsedTarget, ParseError> Parse(bool withNamespace)
    {
        // Only a loop binds slots: a `set` target (the one with namespace attributes) needs
        // no places, and a long one is not searched name by name
        m_givePlaces = !withNamespace;
        if (auto result = ParseTarget(withNamespace, false); !result)
        {
            return MakeUnexpected(result.error());
        }
        return ParsedTarget{ m_nodes.MakeSpan(m_targets), m_namesLoop };
    }

private:
    // A target at the end of m_targets
    Result ParseTarget(bool withNamespace, bool inParens)
    {
        DepthGuard depthGuard(m_depth);
        if (depthGuard.Exceeds(m_maxDepth) || StackNearlyExhausted())
        {
            return MakeParseError(ErrorCode::RecursionLimitExceeded, m_lexer.PeekNextToken());
        }
        // A tuple's node, taken back unless a comma follows the first item
        const std::size_t root = m_targets.size();
        TargetNode tuple;
        tuple.isTuple = true;
        m_targets.push_back(tuple);
        // `()` is an empty tuple
        if (inParens && m_lexer.PeekNextToken() == ')')
        {
            return {};
        }
        std::uint32_t count = 0;
        bool hasComma = false;
        for (;;)
        {
            auto tok = m_lexer.PeekNextToken();
            Result item;
            if (tok == '(')
            {
                item = ParseParenthesized();
            }
            else if (tok == Token::Identifier)
            {
                item = ParseName(tok, withNamespace);
            }
            // A trailing comma is allowed only inside parentheses: `(a,)`, not `set a, = ...`.
            // After `for a,` the caller reports what it expected instead
            else if ((inParens && hasComma && tok == ')') || (!inParens && hasComma && !withNamespace))
            {
                break;
            }
            else
            {
                return MakeParseError(ErrorCode::ExpectedIdentifier, tok);
            }
            if (!item)
            {
                return item;
            }
            ++count;
            // One target, no tuple: the common `set x =` and `for x in`
            if (!hasComma && m_lexer.PeekNextToken() != ',')
            {
                m_targets.erase(m_targets.begin() + static_cast<std::ptrdiff_t>(root));
                return {};
            }
            if (!m_lexer.EatIfEqual(','))
            {
                break;
            }
            hasComma = true;
        }
        auto& node = m_targets[root];
        node.count = count;
        node.size = static_cast<std::uint32_t>(m_targets.size() - root);
        return {};
    }

    // `(...)`: a nested target
    Result ParseParenthesized()
    {
        m_lexer.NextToken();
        if (auto inner = ParseTarget(false, true); !inner)
        {
            return inner;
        }
        if (!m_lexer.EatIfEqual(')'))
        {
            return MakeParseError(ErrorCode::ExpectedRoundBracket, m_lexer.PeekNextToken());
        }
        return {};
    }

    // `name`, or `name.attr` when namespace attributes are allowed
    Result ParseName(const Token& tok, bool withNamespace)
    {
        m_lexer.NextToken();
        const auto name = m_lexer.GetAsString(tok);
        TargetNode item;
        item.name = m_nodes.MakeText(name);
        item.hash = HashedName::Hash(name);
        if (withNamespace && m_lexer.EatIfEqual('.'))
        {
            auto attrTok = m_lexer.NextToken();
            if (attrTok != Token::Identifier)
            {
                return MakeParseError(ErrorCode::ExpectedIdentifier, attrTok);
            }
            item.attr = m_nodes.MakeText(m_lexer.GetAsString(attrTok));
        }
        else
        {
            if (m_givePlaces)
            {
                item.slot = PlaceOf(item);
            }
            m_namesLoop = m_namesLoop || name == "loop";
        }
        m_targets.push_back(item);
        return {};
    }

    // The name's place among the distinct names of the target, from 1
    SlotIndex PlaceOf(const TargetNode& item)
    {
        const auto text = m_nodes.Text(item.name);
        const auto found = std::find_if(m_names.begin(), m_names.end(), [&item, text](const HashedName& name) { return name.hash == item.hash && name.name == text; });
        const auto place = static_cast<std::size_t>(found - m_names.begin()) + 1;
        if (found == m_names.end())
        {
            // The texts stay where they are until the tree is sealed
            m_names.push_back({ text, item.hash });
        }
        return SlotIndex{ static_cast<std::uint16_t>(std::min<std::size_t>(place, SlotIndex::Dynamic)) };
    }

    LexScanner& m_lexer;
    NodeArena& m_nodes;
    boost::container::small_vector<TargetNode, 4> m_targets;
    boost::container::small_vector<HashedName, 4> m_names;
    unsigned m_depth = 0;
    unsigned m_maxDepth;
    bool m_givePlaces = false;
    bool m_namesLoop = false;
};
} // namespace

// Jinja2's parse_assign_target: a name, or names and parenthesised targets separated by
// commas (`a, (b, c)`); for `set` the names outside parentheses can also be namespace
// attributes (`ns.attr`)
nonstd::expected<ParsedTarget, ParseError> StatementsParser::ParseAssignTarget(LexScanner& lexer, bool withNamespace)
{
    return AssignTargetParser(lexer, m_nodes, LimitOrNone(m_settings.limits.maxExpressionDepth)).Parse(withNamespace);
}

// `break` and `continue` belong to the innermost loop of the same function: a macro, call
// or block body starts a new one, and a loop's `else` body is outside the loop
StatementsParser::ParseResult StatementsParser::ParseLoopControl(StatementInfoList& statementsInfo, const Token& stmtTok, LoopControl control)
{
    bool inLoop = false;
    bool inElse = false;
    for (auto p = statementsInfo.rbegin(); p != statementsInfo.rend() && !inLoop; ++p)
    {
        switch (p->type)
        {
        case StatementInfo::ForStatement:
            inLoop = !inElse;
            break;
        case StatementInfo::MacroStatement:
        case StatementInfo::MacroCallStatement:
        case StatementInfo::BlockStatement:
        case StatementInfo::TemplateRoot:
            return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
        default:
            break;
        }
        inElse = p->type == StatementInfo::ElseIfStatement;
    }
    if (!inLoop)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.back().body.emplace_back(m_nodes.Make<LoopControlStatement>(control));
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndFor(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    NodeRef<IRendererBase> elseRenderer;
    if (statementsInfo.back().type == StatementInfo::ElseIfStatement)
    {
        auto elseInfo = PopStatement(statementsInfo);
        // The body of a frame of its own (ParseElse)
        m_nodes.Get<ElseBranchStatement>(elseInfo.renderer).SetMainBody(m_names.MayBindAtEntry(elseInfo.frame) ? TakeFrameBody(elseInfo) : TakeBody(elseInfo));
        elseRenderer = elseInfo.renderer;
    }

    if (statementsInfo.back().type != StatementInfo::ForStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& renderer = m_nodes.Get<ForStatement>(info.renderer);
    renderer.SetMainBody(TakeBody(info));
    if (elseRenderer)
    {
        renderer.SetElseBody(elseRenderer);
    }

    statementsInfo.back().body.push_back(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseIf(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    const auto& pivotTok = lexer.PeekNextToken();
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    // Jinja2 checks the filters and tests of the condition when they run, as those of the branches
    CheckGuard deferred(m_unknown, UnknownCallables::Check::Deferred);
    auto valueExpr = exprParser.ParseTupleOrExpression(lexer);
    if (!valueExpr)
    {
        return MakeParseError(ErrorCode::ExpectedExpression, pivotTok);
    }

    auto renderer = m_nodes.Make<IfStatement>(*valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::IfStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

namespace
{

// Jinja2: `else` ends an `if`, `elif` or `for` body, and nothing may follow it but the end tag
bool IsElseBranch(const NodeArena& nodes, const StatementInfo& info)
{
    return info.type == StatementInfo::ElseIfStatement && nodes.Get<ElseBranchStatement>(info.renderer).IsElse();
}

} // namespace

StatementsParser::ParseResult StatementsParser::ParseElse(LexScanner& /*lexer*/, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto& prev = statementsInfo.back();
    if ((prev.type != StatementInfo::IfStatement && prev.type != StatementInfo::ElseIfStatement && prev.type != StatementInfo::ForStatement) || IsElseBranch(m_nodes, prev))
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto renderer = m_nodes.Make<ElseBranchStatement>(NodeRef<Expression>());
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, stmtTok);
    statementInfo.renderer = renderer;
    // A loop's `else` body is outside the loop, in a frame of its own, as in Jinja2
    // (docs/tasks/0202)
    statementInfo.frame = prev.type == StatementInfo::ForStatement ? m_names.PushWith(m_names.Parent(prev.frame)) : prev.frame;
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseElIf(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto& prev = statementsInfo.back();
    if ((prev.type != StatementInfo::IfStatement && prev.type != StatementInfo::ElseIfStatement) || IsElseBranch(m_nodes, prev))
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    const auto& pivotTok = lexer.PeekNextToken();
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto valueExpr = exprParser.ParseTupleOrExpression(lexer);
    if (!valueExpr)
    {
        return MakeParseError(ErrorCode::ExpectedExpression, pivotTok);
    }

    auto renderer = m_nodes.Make<ElseBranchStatement>(*valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndIf(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);

    // From the last branch to the first, as the statement stack holds them
    boost::container::small_vector<NodeRef<ElseBranchStatement>, 4> elseBranches;

    auto errorTok = stmtTok;
    while (info.type != StatementInfo::IfStatement)
    {
        if (info.type != StatementInfo::ElseIfStatement)
        {
            return MakeParseError(ErrorCode::UnexpectedStatement, errorTok);
        }

        const auto elseRenderer = m_nodes.As<ElseBranchStatement>(info.renderer);
        m_nodes[elseRenderer].SetMainBody(TakeBody(info));

        elseBranches.push_back(elseRenderer);
        errorTok = info.token;
        info = PopStatement(statementsInfo);
    }

    auto& renderer = m_nodes.Get<IfStatement>(info.renderer);
    renderer.SetMainBody(TakeBody(info));
    std::reverse(elseBranches.begin(), elseBranches.end());
    renderer.SetElseBranches(m_nodes.MakeSpan(elseBranches));

    statementsInfo.back().body.push_back(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseSet(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto targetTok = lexer.PeekNextToken();
    auto target = ParseAssignTarget(lexer, true);
    if (!target)
    {
        return MakeUnexpected(target.error());
    }
    // Jinja2: "Can't assign to special loop variable in for-loop target", for a `set` anywhere
    // in a loop, a macro or block defined there included
    if (target->namesLoop && std::any_of(statementsInfo.begin(), statementsInfo.end(), [](const StatementInfo& info) { return info.type == StatementInfo::ForStatement; }))
    {
        return MakeParseError(ErrorCode::UnexpectedToken, targetTok);
    }
    const auto vars = target->target;

    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    if (lexer.EatIfEqual('='))
    {
        const auto expr = exprParser.ParseTupleOrExpression(lexer);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        // Stored once the value is read, which may read the name itself
        m_names.AddStores(m_names.Current(), m_nodes[vars], m_nodes);
        statementsInfo.back().body.emplace_back(
            m_nodes.Make<SetLineStatement>(vars, *expr));
    }
    else if (lexer.EatIfEqual('|'))
    {
        // The filters of a set block are in its frame, which Jinja2 checks even in an `if`
        CheckGuard checked(m_unknown, UnknownCallables::Check::Now);
        const auto expr = exprParser.ParseFilterExpression(lexer);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        auto statementInfo = StatementInfo::Create(
            StatementInfo::SetStatement, stmtTok);
        statementInfo.renderer = m_nodes.Make<SetFilteredBlockStatement>(
            vars, *expr);
        statementInfo.frame = m_names.PushWith(m_names.Current());
        statementsInfo.push_back(std::move(statementInfo));
    }
    else
    {
        auto operTok = lexer.NextToken();
        if (lexer.NextToken() != Token::Eof)
        {
            return MakeParseError(ErrorCode::YetUnsupported, operTok, { stmtTok });
        }
        auto statementInfo = StatementInfo::Create(
            StatementInfo::SetStatement, stmtTok);
        statementInfo.renderer = m_nodes.Make<SetRawBlockStatement>(
            vars);
        statementInfo.frame = m_names.PushWith(m_names.Current());
        statementsInfo.push_back(std::move(statementInfo));
    }

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseEndSet(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (statementsInfo.back().type != StatementInfo::SetStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& statement = m_nodes.Get<SetBlockStatement>(info.renderer);
    statement.SetBody(TakeBody(info));
    // Stored once the body has rendered, which may read the name itself
    const auto& outer = statementsInfo.back();
    const auto kind = StatementInfo::OwnsFrame(outer.type) ? NameResolver::StoreKind::Store : NameResolver::StoreKind::Branch;
    m_names.AddStores(outer.frame, m_nodes[statement.GetTarget()], m_nodes, kind);
    statementsInfo.back().body.push_back(info.renderer);

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseBlock(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    Token nextTok = lexer.NextToken();
    if (nextTok != Token::Identifier)
    {
        return MakeParseError(ErrorCode::ExpectedIdentifier, nextTok);
    }

    const auto blockName = m_nodes.MakeText(lexer.GetAsString(nextTok));

    // Jinja2 accepts `scoped`, then `required`, in this order
    bool isScoped = lexer.EatIfEqual(Keyword::Scoped);
    bool isRequired = false;
    Token modifierTok = lexer.PeekNextToken();
    if (modifierTok == Token::Identifier && lexer.GetAsString(modifierTok) == "required")
    {
        lexer.EatToken();
        isRequired = true;
    }
    modifierTok = lexer.PeekNextToken();
    if (modifierTok != Token::Eof)
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, modifierTok, Token::Eof);
    }

    auto blockRenderer = m_nodes.Make<BlockStatement>(blockName, isScoped, isRequired);
    if (!m_root.AddBlock(m_nodes, blockRenderer))
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::BlockStatement, stmtTok);
    statementInfo.frame = m_names.PushUnit(m_names.Current(), blockRenderer);
    statementInfo.renderer = std::move(blockRenderer);
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndBlock(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (statementsInfo.back().type != StatementInfo::BlockStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& blockStmt = m_nodes.Get<BlockStatement>(info.renderer);
    // `endblock` may repeat the name of the block it ends, and only that name
    Token nextTok = lexer.PeekNextToken();
    if (nextTok == Token::Identifier && lexer.GetAsString(nextTok) == blockStmt.GetName(m_nodes))
    {
        lexer.EatToken();
    }

    blockStmt.SetMainBody(TakeBody(info));
    statementsInfo.back().body.push_back(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseExtends(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (!m_env)
    {
        return MakeParseError(ErrorCode::TemplateEnvAbsent, stmtTok);
    }

    // Jinja2 allows `extends` at the top level only, which `if` does not leave
    for (auto& info : statementsInfo)
    {
        if (info.type != StatementInfo::TemplateRoot && info.type != StatementInfo::IfStatement && info.type != StatementInfo::ElseIfStatement)
        {
            return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
        }
    }

    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }

    auto renderer = m_nodes.Make<ExtendsStatement>(*expr);
    statementsInfo.back().body.emplace_back(renderer);
    m_root.hasExtends = true;
    if (statementsInfo.size() == 1)
    {
        m_root.knownExtends = true;
    }

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseMacro(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    Token nextTok = lexer.NextToken();
    if (nextTok != Token::Identifier)
    {
        return MakeParseError(ErrorCode::ExpectedIdentifier, nextTok);
    }

    std::string macroName = lexer.GetAsString(nextTok);
    // The parameters' defaults count with the body
    const auto marks = m_nodes.Marks();
    const auto outerFrame = m_names.Current();
    // In the arena now: the resolver keeps a view of the name
    const auto nameText = m_nodes.MakeText(macroName);
    m_names.AddStore(outerFrame, m_nodes.Text(nameText));

    // Jinja2 requires the parameter list, even an empty one
    if (!lexer.EatIfEqual('('))
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::LBracket);
    }
    auto result = ParseMacroParams(lexer);
    if (!result)
    {
        return MakeUnexpected(result.error());
    }

    auto renderer = m_nodes.Make<MacroStatement>(m_nodes, nameText, result.value());
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::MacroStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementInfo.marks = marks;
    statementInfo.frame = m_names.PushMacro(outerFrame, renderer);
    statementsInfo.push_back(std::move(statementInfo));

    return ParseResult();
}

namespace
{
using MacroDefaultTokens = std::pair<TokensList::const_iterator, TokensList::const_iterator>;

// Does a default name an argument of this macro or a special one (an attribute `x.a` does not count)?
void MarkDefaultsReferringToArgs(MacroParamsInfo& items, const std::vector<MacroDefaultTokens>& defaultTokens, const LexScanner& lexer)
{
    auto isArgName = [&items](const std::string& name) {
        if (name == "caller" || name == "varargs" || name == "kwargs")
        {
            return true;
        }
        return std::any_of(items.begin(), items.end(), [&name](const MacroParamInfo& p) { return p.paramName == name; });
    };
    for (std::size_t idx = 0; idx < items.size(); ++idx)
    {
        const auto& range = defaultTokens[idx];
        for (auto t = range.first; t != range.second && !items[idx].defaultRefersToArgs; ++t)
        {
            bool isAttribute = t != range.first && *std::prev(t) == '.';
            if (t->type == Token::Identifier && !isAttribute && isArgName(lexer.GetAsString(*t)))
            {
                items[idx].defaultRefersToArgs = true;
            }
        }
    }
}
} // namespace

nonstd::expected<MacroParamsInfo, ParseError> StatementsParser::ParseMacroParams(LexScanner& lexer)
{
    MacroParamsInfo items;

    if (lexer.EatIfEqual(')'))
    {
        return items;
    }
    // The defaults are in the macro's frame, which Jinja2 checks even in an `if`
    CheckGuard checked(m_unknown, UnknownCallables::Check::Now);

    std::vector<MacroDefaultTokens> defaultTokens;

    // Defaults are evaluated where the macro is defined or inside its call: both stay
    // lookups by name
    const auto outerFrame = m_names.Current();
    m_names.SetCurrent(m_names.PushDynamic(outerFrame));
    class RestoreFrame
    {
    public:
        RestoreFrame(NameResolver& names, NameResolver::FrameId frame)
            : m_names(names)
            , m_frame(frame)
        {
        }
        RestoreFrame(const RestoreFrame&) = delete;
        RestoreFrame(RestoreFrame&&) = delete;
        RestoreFrame& operator=(const RestoreFrame&) = delete;
        RestoreFrame& operator=(RestoreFrame&&) = delete;
        ~RestoreFrame() { m_names.SetCurrent(m_frame); }

    private:
        NameResolver& m_names;
        NameResolver::FrameId m_frame;
    };
    const RestoreFrame restoreFrame(m_names, outerFrame);

    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    do
    {
        Token name = lexer.NextToken();
        if (name != Token::Identifier)
        {
            return MakeParseError(ErrorCode::ExpectedIdentifier, name);
        }

        auto paramName = lexer.GetAsString(name);
        auto isSameName = [&paramName](const MacroParamInfo& p) { return p.paramName == paramName; };
        if (std::any_of(items.begin(), items.end(), isSameName))
        {
            return MakeParseError(ErrorCode::UnexpectedToken, name);
        }

        NodeRef<Expression> defVal;
        auto defaultBegin = lexer.GetState().m_cur;
        if (lexer.EatIfEqual('='))
        {
            defaultBegin = lexer.GetState().m_cur;
            exprParser.NextTopLevelExpression();
            auto result = exprParser.ParseFullExpression(lexer, false);
            if (!result)
            {
                return MakeUnexpected(result.error());
            }

            defVal = *result;
        }
        else if (!items.empty() && items.back().defaultValue)
        {
            // non-default argument follows default argument
            return MakeParseError(ErrorCode::UnexpectedToken, name);
        }

        defaultTokens.emplace_back(defaultBegin, lexer.GetState().m_cur);

        MacroParamInfo p;
        p.paramName = std::move(paramName);
        p.defaultValue = std::move(defVal);
        items.push_back(std::move(p));

    } while (lexer.EatIfEqual(','));

    auto tok = lexer.NextToken();
    if (tok != ')')
    {
        return MakeParseError(ErrorCode::ExpectedRoundBracket, tok);
    }

    MarkDefaultsReferringToArgs(items, defaultTokens, lexer);

    return items;
}

StatementsParser::ParseResult StatementsParser::ParseEndMacro(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (statementsInfo.back().type != StatementInfo::MacroStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& renderer = m_nodes.Get<MacroStatement>(info.renderer);
    // Jinja2: the special "caller" argument must be omitted or be given a default
    if (renderer.HasInvalidCallerParam())
    {
        return MakeParseError(ErrorCode::UnexpectedToken, info.token);
    }
    renderer.SetMainBody(TakeBody(info));
    renderer.SetBodyMarks(info.marks, m_nodes.Marks());

    statementsInfo.back().body.push_back(info.renderer);

    return ParseResult();
}

namespace
{
// The `caller=` keyword of the outermost call in [first, last): the last call at bracket depth 0
// takes the arguments, `caller` among them at depth 1
std::optional<Token> FindCallerKeyword(const LexScanner& lexer, TokensList::const_iterator first, TokensList::const_iterator last)
{
    std::optional<Token> found;
    int depth = 0;
    for (auto it = first; it != last; ++it)
    {
        const Token& tok = *it;
        if (tok == '(' || tok == '[' || tok == '{')
        {
            if (depth == 0)
            {
                found.reset();
            }
            ++depth;
        }
        else if (tok == ')' || tok == ']' || tok == '}')
        {
            --depth;
        }
        else if (depth == 1 && tok == Token::Identifier && lexer.GetAsView(tok) == "caller" && std::next(it) != last && *std::next(it) == '=')
        {
            found = tok;
        }
    }
    return found;
}
} // namespace

StatementsParser::ParseResult StatementsParser::ParseCall(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    MacroParamsInfo callbackParams;
    // The parameters' defaults count with the body, the arguments of the call do not
    auto marks = m_nodes.Marks();

    if (lexer.EatIfEqual('('))
    {
        auto result = ParseMacroParams(lexer);
        if (!result)
        {
            return MakeUnexpected(result.error());
        }

        callbackParams = std::move(result.value());
    }

    // Jinja2 takes any call expression: m(), ns.m(), l[0](), m()()
    const Token exprTok = lexer.PeekNextToken();
    const auto exprBegin = lexer.GetState().m_cur;
    const auto beforeCall = m_nodes.Marks();
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    const auto call = m_nodes.As<CallExpression>(*expr);
    if (!call)
    {
        // `{% call m %}`, `{% call m()|f %}`: Jinja2's "expected call"
        if (exprTok == Token::Identifier)
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::LBracket);
        }
        return MakeParseError(ErrorCode::UnexpectedToken, exprTok);
    }
    // The call block passes `caller` itself: Python rejects the repeated keyword at compile time
    for (const auto& param : m_nodes[m_nodes.Get<CallExpression>(call).GetParams().kwParams])
    {
        if (m_nodes.Text(param.name) == "caller")
        {
            return MakeParseError(ErrorCode::UnexpectedToken, FindCallerKeyword(lexer, exprBegin, lexer.GetState().m_cur).value_or(exprTok));
        }
    }
    const auto afterCall = m_nodes.Marks();
    marks.nameRefs += afterCall.nameRefs - beforeCall.nameRefs;
    marks.contextNodes += afterCall.contextNodes - beforeCall.contextNodes;

    auto renderer = m_nodes.Make<MacroCallStatement>(m_nodes, call, callbackParams);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::MacroCallStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementInfo.marks = marks;
    statementInfo.frame = m_names.PushMacro(m_names.Current(), renderer);
    statementsInfo.push_back(std::move(statementInfo));

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndCall(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (statementsInfo.back().type != StatementInfo::MacroCallStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& renderer = m_nodes.Get<MacroCallStatement>(info.renderer);
    // Jinja2: the special "caller" argument must be omitted or be given a default
    if (renderer.HasInvalidCallerParam())
    {
        return MakeParseError(ErrorCode::UnexpectedToken, info.token);
    }
    renderer.SetMainBody(TakeBody(info));
    renderer.SetBodyMarks(info.marks, m_nodes.Marks());

    statementsInfo.back().body.push_back(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseInclude(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    // auto operTok = lexer.NextToken();
    NodeRef<Expression> valueExpr;
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    valueExpr = *expr;

    Token nextTok = lexer.PeekNextToken();
    bool isIgnoreMissing = false;
    bool isWithContext = true;
    bool hasIgnoreMissing = false;
    if (lexer.EatIfEqual(Keyword::Ignore))
    {
        if (lexer.EatIfEqual(Keyword::Missing))
        {
            isIgnoreMissing = true;
        }
        else
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::Missing);
        }

        hasIgnoreMissing = true;
        nextTok = lexer.PeekNextToken();
    }

    auto kw = nextTok.keyword;
    bool hasContextControl = false;
    if (kw == Keyword::With || kw == Keyword::Without)
    {
        lexer.EatToken();
        isWithContext = kw == Keyword::With;
        if (!lexer.EatIfEqual(Keyword::Context))
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::Context);
        }

        nextTok = lexer.PeekNextToken();
        hasContextControl = true;
    }

    if (nextTok != Token::Eof)
    {
        if (hasContextControl)
        {
            return MakeParseErrorTL(ErrorCode::ExpectedEndOfStatement, nextTok, Token::Eof);
        }

        if (hasIgnoreMissing)
        {
            return MakeParseErrorTL(ErrorCode::UnexpectedToken, nextTok, Token::Eof, Token::With, Token::Without);
        }

        return MakeParseErrorTL(ErrorCode::UnexpectedToken, nextTok, Token::Eof, Token::Ignore, Token::With, Token::Without);
    }

    if (!m_env && !isIgnoreMissing)
    {
        return MakeParseError(ErrorCode::TemplateEnvAbsent, stmtTok);
    }

    auto renderer = m_nodes.Make<IncludeStatement>(isIgnoreMissing, isWithContext);
    m_nodes[renderer].SetIncludeNamesExpr(valueExpr, m_nodes[valueExpr].GetConstant(m_nodes) != nullptr);
    statementsInfo.back().body.emplace_back(renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseImport(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (!m_env)
    {
        return MakeParseError(ErrorCode::TemplateEnvAbsent, stmtTok);
    }

    NodeRef<Expression> valueExpr;
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    valueExpr = *expr;

    if (!lexer.EatIfEqual(Keyword::As))
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::As);
    }

    Token name;
    if (!lexer.EatIfEqual(Token::Identifier, &name))
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::Identifier);
    }

    Token nextTok = lexer.PeekNextToken();
    auto kw = nextTok.keyword;
    bool hasContextControl = false;
    bool isWithContext = false;
    if (kw == Keyword::With || kw == Keyword::Without)
    {
        lexer.EatToken();
        isWithContext = kw == Keyword::With;
        if (!lexer.EatIfEqual(Keyword::Context))
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::Context);
        }

        nextTok = lexer.PeekNextToken();
        hasContextControl = true;
    }

    if (nextTok != Token::Eof)
    {
        if (hasContextControl)
        {
            return MakeParseErrorTL(ErrorCode::ExpectedEndOfStatement, nextTok, Token::Eof);
        }

        return MakeParseErrorTL(ErrorCode::UnexpectedToken, nextTok, Token::Eof, Token::With, Token::Without);
    }

    auto renderer = m_nodes.Make<ImportStatement>(isWithContext);
    m_nodes[renderer].SetImportNameExpr(valueExpr);
    const auto namespaceName = lexer.GetAsString(name);
    const auto namespaceText = m_nodes.MakeText(namespaceName);
    m_names.AddStore(m_names.Current(), m_nodes.Text(namespaceText));
    m_nodes[renderer].SetNamespace(namespaceText, HashedName::Hash(namespaceName));
    statementsInfo.back().body.emplace_back(renderer);

    return ParseResult();
}

namespace
{
// `with context` or `without context` after the imported names; `kw` is the keyword of the next token
bool EatImportContextControl(LexScanner& lexer, Keyword kw, bool& isWithContext)
{
    if (kw != Keyword::With && kw != Keyword::Without)
    {
        return false;
    }
    lexer.NextToken();
    if (lexer.EatIfEqual(Keyword::Context))
    {
        isWithContext = kw == Keyword::With;
        return true;
    }

    lexer.ReturnToken();
    return false;
}

// `name` or `name as alias` of `{% from ... import ... %}`
nonstd::expected<std::pair<std::string, std::string>, ParseError> ParseImportedName(LexScanner& lexer, Token& nextTok)
{
    std::pair<std::string, std::string> macroMap;
    if (!lexer.EatIfEqual(Token::Identifier, &nextTok))
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Identifier);
    }

    macroMap.first = lexer.GetAsString(nextTok);
    // Jinja2: names starting with an underline can not be imported
    if (!macroMap.first.empty() && macroMap.first[0] == '_')
    {
        return MakeParseError(ErrorCode::UnexpectedToken, nextTok);
    }

    if (lexer.EatIfEqual(Keyword::As))
    {
        if (!lexer.EatIfEqual(Token::Identifier, &nextTok))
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Identifier);
        }
        macroMap.second = lexer.GetAsString(nextTok);
    }
    else
    {
        macroMap.second = macroMap.first;
    }
    return macroMap;
}
} // namespace

StatementsParser::ParseResult StatementsParser::ParseFrom(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (!m_env)
    {
        return MakeParseError(ErrorCode::TemplateEnvAbsent, stmtTok);
    }

    NodeRef<Expression> valueExpr;
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    valueExpr = *expr;

    if (!lexer.EatIfEqual(Keyword::Import))
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), Token::Identifier);
    }

    std::vector<std::pair<std::string, std::string>> mappedNames;

    Token nextTok;
    bool hasContextControl = false;
    bool isWithContext = false;

    for (;;)
    {
        bool hasComma = false;
        if (!mappedNames.empty())
        {
            if (!lexer.EatIfEqual(Token::Comma))
            {
                hasComma = true;
            };
        }

        nextTok = lexer.PeekNextToken();
        if (EatImportContextControl(lexer, nextTok.keyword, isWithContext))
        {
            hasContextControl = true;
            nextTok = lexer.PeekNextToken();
            break;
        }

        if (hasComma)
        {
            break;
        }

        auto macroMap = ParseImportedName(lexer, nextTok);
        if (!macroMap)
        {
            return MakeUnexpected(std::move(macroMap.error()));
        }
        mappedNames.push_back(std::move(*macroMap));
    }

    if (nextTok != Token::Eof)
    {
        if (hasContextControl)
        {
            return MakeParseErrorTL(ErrorCode::ExpectedEndOfStatement, nextTok, Token::Eof);
        }

        if (mappedNames.empty())
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Eof, Token::Identifier);
        }
        return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Eof, Token::Comma, Token::With, Token::Without);
    }

    auto renderer = m_nodes.Make<ImportStatement>(isWithContext);
    m_nodes[renderer].SetImportNameExpr(valueExpr);

    // Every `name as alias` pair is kept in source order; a name imported twice binds both aliases
    std::vector<ImportName> names;
    names.reserve(mappedNames.size());
    for (const auto& nameInfo : mappedNames)
    {
        const auto& name = names.emplace_back(ImportName{ m_nodes.MakeText(nameInfo.first), HashedName::Hash(nameInfo.first), m_nodes.MakeText(nameInfo.second), HashedName::Hash(nameInfo.second) });
        // Jinja2 stores the alias only; the imported name still hides a slot of its own
        m_names.AddStore(m_names.Current(), m_nodes.Text(name.alias));
        if (nameInfo.first != nameInfo.second)
        {
            m_names.AddStore(m_names.Current(), m_nodes.Text(name.name), NameResolver::StoreKind::Branch);
        }
    }
    m_nodes[renderer].SetNamesToImport(m_nodes.MakeSpan(names));

    statementsInfo.back().body.emplace_back(renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseDo(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& /*stmtTok*/)
{
    NodeRef<Expression> valueExpr;
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    valueExpr = *expr;

    auto renderer = m_nodes.Make<DoStatement>(valueExpr);
    statementsInfo.back().body.emplace_back(renderer);

    return jinja2::StatementsParser::ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseWith(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    std::vector<NamedExpr> vars;

    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    while (lexer.PeekNextToken() == Token::Identifier)
    {
        auto nameTok = lexer.NextToken();
        if (!lexer.EatIfEqual('='))
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), '=');
        }

        exprParser.NextTopLevelExpression();
        auto expr = exprParser.ParseFullExpression(lexer);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        auto valueExpr = *expr;

        const auto name = lexer.GetAsString(nameTok);
        vars.push_back({ m_nodes.MakeText(name), HashedName::Hash(name), valueExpr });

        if (!lexer.EatIfEqual(','))
        {
            break;
        }
    }

    // {% with %} without assignments only opens a scope
    auto nextTok = lexer.PeekNextToken();
    if (nextTok != Token::Eof)
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Eof, ',');
    }

    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::WithStatement, stmtTok);
    statementInfo.frame = m_names.PushWith(m_names.Current());
    for (const auto& var : vars)
    {
        m_names.AddStore(statementInfo.frame, m_nodes.Text(var.name), NameResolver::StoreKind::Target);
    }
    auto renderer = m_nodes.Make<WithStatement>();
    m_nodes[renderer].SetScopeVars(m_nodes.MakeSpan(vars));
    statementInfo.renderer = renderer;
    statementsInfo.push_back(std::move(statementInfo));

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndWith(LexScanner& /*lexer*/, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (statementsInfo.back().type != StatementInfo::WithStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& renderer = m_nodes.Get<WithStatement>(info.renderer);
    renderer.SetMainBody(TakeBody(info));

    statementsInfo.back().body.push_back(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseFilter(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    // The filters of a filter block are in its frame, which Jinja2 checks even in an `if`
    CheckGuard checked(m_unknown, UnknownCallables::Check::Now);
    auto filterExpr = exprParser.ParseFilterExpression(lexer);
    if (!filterExpr)
    {
        return MakeUnexpected(filterExpr.error());
    }

    auto renderer = m_nodes.Make<FilterStatement>(*filterExpr);
    auto statementInfo = StatementInfo::Create(
        StatementInfo::FilterStatement, stmtTok);
    statementInfo.renderer = std::move(renderer);
    statementInfo.frame = m_names.PushWith(m_names.Current());
    statementsInfo.push_back(std::move(statementInfo));

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseEndFilter(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (statementsInfo.back().type != StatementInfo::FilterStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& renderer = m_nodes.Get<FilterStatement>(info.renderer);
    renderer.SetBody(TakeBody(info));

    statementsInfo.back().body.push_back(info.renderer);

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseAutoescape(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    // Jinja2 checks the value of `autoescape` even in an `if`
    CheckGuard checked(m_unknown, UnknownCallables::Check::Now);
    auto valueExpr = exprParser.ParseFullExpression(lexer);
    if (!valueExpr)
    {
        return MakeUnexpected(valueExpr.error());
    }

    auto renderer = m_nodes.Make<AutoescapeStatement>(*valueExpr);
    auto statementInfo = StatementInfo::Create(StatementInfo::AutoescapeStatement, stmtTok);
    statementInfo.renderer = std::move(renderer);
    statementInfo.frame = m_names.PushWith(m_names.Current());
    statementsInfo.push_back(std::move(statementInfo));

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseEndAutoescape(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    if (statementsInfo.back().type != StatementInfo::AutoescapeStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = PopStatement(statementsInfo);
    auto& renderer = m_nodes.Get<AutoescapeStatement>(info.renderer);
    renderer.SetBody(TakeBody(info));

    statementsInfo.back().body.push_back(info.renderer);

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto trans = std::make_shared<TransInfo>();
    Token contextTok;
    if (lexer.EatIfEqual(Token::String, &contextTok))
    {
        trans->context = lexer.GetValue(contextTok);
    }

    // The parameters, as Jinja2's InternationalizationExtension.parse reads them
    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    while (lexer.PeekNextToken() != Token::Eof)
    {
        if (!trans->variables.empty() && !lexer.EatIfEqual(','))
        {
            return MakeParseErrorTL(ErrorCode::ExpectedToken, lexer.PeekNextToken(), ',');
        }
        // A colon ends them, for Python compatibility
        if (lexer.EatIfEqual(':'))
        {
            break;
        }
        auto nameTok = lexer.NextToken();
        if (nameTok != Token::Identifier)
        {
            return MakeParseError(ErrorCode::ExpectedIdentifier, nameTok);
        }
        auto name = lexer.GetAsString(nameTok);
        // Jinja2: translatable variable defined twice
        if (trans->HasVariable(name))
        {
            return MakeParseError(ErrorCode::UnexpectedToken, nameTok);
        }

        NodeRef<Expression> value;
        if (lexer.EatIfEqual('='))
        {
            exprParser.NextTopLevelExpression();
            auto expr = exprParser.ParseFullExpression(lexer);
            if (!expr)
            {
                return MakeUnexpected(expr.error());
            }
            value = *expr;
        }
        else if (!trans->trimmed && (name == "trimmed" || name == "notrimmed"))
        {
            trans->trimmed = name == "trimmed";
            continue;
        }
        else
        {
            value = ValueRefExpression::Make(m_nodes, name);
        }
        trans->variables.emplace_back(name, std::move(value));
        trans->paramsCount = trans->variables.size();
        if (trans->pluralVar.empty())
        {
            trans->pluralVar = name;
        }
    }

    auto statementInfo = StatementInfo::Create(StatementInfo::TransStatement, stmtTok);
    statementInfo.trans = std::move(trans);
    statementsInfo.push_back(std::move(statementInfo));
    return {};
}

StatementsParser::ParseResult StatementsParser::ParseInTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    // Jinja2: control structures in translatable sections are not allowed, and trans blocks can't be nested
    auto name = stmtTok == Token::Identifier ? lexer.GetAsString(stmtTok) : std::string();
    if (name == "pluralize")
    {
        return ParsePluralize(lexer, statementsInfo, stmtTok);
    }
    if (name == "endtrans")
    {
        return ParseEndTrans(lexer, statementsInfo, stmtTok);
    }
    return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
}

StatementsParser::ParseResult StatementsParser::ParsePluralize(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto& trans = *statementsInfo.back().trans;
    // Jinja2: a translatable section can have only one pluralize section
    if (trans.hasPlural)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    // Without parameters the first name in the singular message selects the plural form
    if (trans.pluralVar.empty() && !trans.singularNames.empty())
    {
        trans.pluralVar = trans.singularNames.front();
    }
    trans.hasPlural = true;

    if (lexer.PeekNextToken() != Token::Eof)
    {
        auto nameTok = lexer.NextToken();
        if (nameTok != Token::Identifier)
        {
            return MakeParseError(ErrorCode::ExpectedIdentifier, nameTok);
        }
        // Jinja2: unknown variable for pluralization
        auto name = lexer.GetAsString(nameTok);
        if (!trans.HasParam(name))
        {
            return MakeParseError(ErrorCode::UnexpectedToken, nameTok);
        }
        trans.pluralVar = name;
    }
    return {};
}

namespace
{
// Jinja2's _trim_whitespace: strips the message and joins its lines with single spaces
template<typename CharT>
std::basic_string<CharT> TrimTransMessage(const std::basic_string<CharT>& message)
{
    auto isSpace = [](CharT ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v'; };
    std::basic_string<CharT> result;
    size_t pos = 0;
    size_t end = message.size();
    while (pos < end && isSpace(message[pos]))
    {
        ++pos;
    }
    while (end > pos && isSpace(message[end - 1]))
    {
        --end;
    }
    while (pos < end)
    {
        if (!isSpace(message[pos]))
        {
            result.push_back(message[pos++]);
            continue;
        }
        // A run of whitespace becomes one space when it has a newline in it
        auto runEnd = pos;
        bool hasNewline = false;
        for (; runEnd < end && isSpace(message[runEnd]); ++runEnd)
        {
            hasNewline = hasNewline || message[runEnd] == '\n';
        }
        if (hasNewline)
        {
            result.push_back(' ');
        }
        else
        {
            result.append(message, pos, runEnd - pos);
        }
        pos = runEnd;
    }
    return result;
}

// The names the messages use become variables too
void AddTransMessageNames(NodeArena& nodes, TransInfo& trans)
{
    for (auto* names : { &trans.singularNames, &trans.pluralNames })
    {
        for (auto& name : *names)
        {
            if (!trans.HasVariable(name))
            {
                trans.variables.emplace_back(name, ValueRefExpression::Make(nodes, name));
            }
        }
    }
}

void TrimTransMessages(TransInfo& trans)
{
    for (auto* message : { &trans.singular, &trans.plural })
    {
        if (auto* narrow = std::get_if<std::string>(message))
        {
            *narrow = TrimTransMessage(*narrow);
        }
        else if (auto* wide = std::get_if<std::wstring>(message))
        {
            *wide = TrimTransMessage(*wide);
        }
    }
}
} // namespace

StatementsParser::ParseResult StatementsParser::ParseEndTrans(LexScanner& /*lexer*/, StatementInfoList& statementsInfo, const Token& /*stmtTok*/)
{
    auto info = PopStatement(statementsInfo);
    auto& trans = *info.trans;

    // Jinja2: pluralize without variables
    if (trans.hasPlural && trans.pluralVar.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, info.token);
    }

    AddTransMessageNames(m_nodes, trans);

    if (trans.trimmed.value_or(false))
    {
        TrimTransMessages(trans);
    }

    // gettext(singular, **variables), with the `n` and `p` variants Jinja2 uses for a plural form
    // and a message context; ngettext takes `num` from the count unless the count is `num` itself
    CallParamsInfo params;
    if (!trans.context.IsUndefined())
    {
        params.posParams.emplace_back(MakeConstant(m_nodes, trans.context));
    }
    params.posParams.emplace_back(MakeConstant(m_nodes, InternalValue(trans.singular)));
    std::string fnName = trans.context.IsUndefined() ? "gettext" : "pgettext";
    for (size_t idx = 0; idx < trans.variables.size(); ++idx)
    {
        auto& name = trans.variables[idx].first;
        auto slot = ValueRefExpression::Make(m_nodes, TransStatement::VariableSlot(idx));
        if (trans.hasPlural && name == trans.pluralVar)
        {
            params.posParams.emplace_back(MakeConstant(m_nodes, InternalValue(trans.plural)));
            params.posParams.push_back(slot);
            fnName = trans.context.IsUndefined() ? "ngettext" : "npgettext";
            if (name == "num")
            {
                continue;
            }
        }
        params.kwParams[name] = slot;
    }

    ExpressionParser exprParser(m_settings, m_env, m_nodes, m_names, m_unknown);
    auto call = m_nodes.Make<CallExpression>(m_nodes, ValueRefExpression::Make(m_nodes, fnName), std::move(params));
    auto output = MakeExpressionRenderer(m_nodes, call, exprParser.GetFinalize());
    std::vector<NodeRef<Expression>> values;
    values.reserve(trans.variables.size());
    for (const auto& var : trans.variables)
    {
        values.push_back(var.second);
    }
    statementsInfo.back().body.emplace_back(m_nodes.Make<TransStatement>(m_nodes.MakeSpan(values), output));
    return {};
}

template<typename CharT>
InternalValue EscapedLiteralValue(std::basic_string_view<CharT> text, const std::string& newlineSequence, bool& malformed)
{
    // Jinja2 applies newline_sequence to the literal newlines of a string before decoding its
    // escapes. Converted on the stack: the value is the one allocation (docs/tasks/0151)
    boost::container::small_vector<CharT, 256> chars;
    for (auto ch : text)
    {
        if (ch == '\n')
        {
            chars.insert(chars.end(), newlineSequence.begin(), newlineSequence.end());
        }
        else
        {
            chars.push_back(ch);
        }
    }
    bool ok = true;
    auto end = CompileEscapes(chars.begin(), chars.end(), &ok);
    malformed = !ok;
    return InternalValue::FromText(std::basic_string_view<CharT>(chars.data(), static_cast<size_t>(end - chars.begin())));
}

template InternalValue EscapedLiteralValue<char>(std::string_view text, const std::string& newlineSequence, bool& malformed);
template InternalValue EscapedLiteralValue<wchar_t>(std::wstring_view text, const std::string& newlineSequence, bool& malformed);

} // namespace jinja2
