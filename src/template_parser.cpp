#include "template_parser.h"

#include "error_handling.h"
#include "expression_evaluator.h"
#include "expression_parser.h"
#include "internal_value.h"
#include "lexer.h"
#include "make_unexpected.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "renderer.h"
#include "statements.h"

#include <jinja2cpp/error_info.h>

#include <boost/cast.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{

StatementsParser::ParseResult StatementsParser::Parse(LexScanner& lexer, StatementInfoList& statementsInfo)
{
    const auto& tok = lexer.NextToken();
    ParseResult result;

    auto keyword = tok.keyword;
    // Jinja2: required blocks can only contain comments or whitespace
    if (keyword != Keyword::EndBlock && !statementsInfo.empty() && statementsInfo.back().type == StatementInfo::BlockStatement && std::static_pointer_cast<BlockStatement>(statementsInfo.back().renderer)->IsRequired())
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

    if (result)
    {
        // Each open block is a level of render recursion; elif and else branches are not
        const auto openBlocks = std::count_if(statementsInfo.begin(), statementsInfo.end(), [](const StatementInfo& info) {
            return info.type != StatementInfo::ElseIfStatement;
        });
        if (static_cast<std::size_t>(openBlocks) > MaxBlockNesting)
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
    if (tok == Token::Identifier && (AsString(tok.value) == "break" || AsString(tok.value) == "continue"))
    {
        if (!m_settings.extensions.loopControls)
        {
            return MakeParseError(ErrorCode::ExtensionDisabled, tok);
        }
        return ParseLoopControl(statementsInfo, tok, AsString(tok.value) == "break" ? LoopControl::Break : LoopControl::Continue);
    }
    if (m_settings.extensions.i18n && tok == Token::Identifier && AsString(tok.value) == "trans")
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

StatementsParser::ParseResult StatementsParser::ParseFor(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto targetTok = lexer.PeekNextToken();
    auto target = ParseAssignTarget(lexer, false);
    if (!target)
    {
        return MakeUnexpected(target.error());
    }
    // Jinja2: "Can't assign to special loop variable in for-loop target"
    std::function<bool(const AssignTarget&)> namesLoop = [&namesLoop](const AssignTarget& t) {
        return t.name == "loop" || std::any_of(t.items.begin(), t.items.end(), namesLoop);
    };
    if (namesLoop(*target))
    {
        return MakeParseError(ErrorCode::UnexpectedToken, targetTok);
    }

    if (!lexer.EatIfEqual(Keyword::In))
    {
        const Token& tok1 = lexer.PeekNextToken();
        Token tok2 = tok1;
        tok2.type = Token::Identifier;
        tok2.range.endOffset = tok2.range.startOffset;
        tok2.value = InternalValue();
        return MakeParseErrorTL(ErrorCode::ExpectedToken, tok1, tok2, Token::In, ',');
    }

    ExpressionParser exprPraser(m_settings, m_env);
    auto valueExpr = exprPraser.ParseTupleOrExpression(lexer, false);
    if (!valueExpr)
    {
        return MakeUnexpected(valueExpr.error());
    }

    Token flagsTok;
    bool isRecursive = false;
    if (lexer.EatIfEqual(Keyword::Recursive, &flagsTok))
    {
        isRecursive = true;
    }

    ExpressionEvaluatorPtr<> ifExpr;
    if (lexer.EatIfEqual(Keyword::If))
    {
        auto parsedExpr = exprPraser.ParseFullExpression(lexer, false);
        if (!parsedExpr)
        {
            return MakeUnexpected(parsedExpr.error());
        }
        ifExpr = *parsedExpr;
    }
    else if (lexer.PeekNextToken() != Token::Eof)
    {
        const auto& tok1 = lexer.PeekNextToken();
        return MakeParseErrorTL(ErrorCode::ExpectedToken, tok1, Token::If, Token::Recursive, Token::Eof);
    }

    auto renderer = std::make_shared<ForStatement>(std::move(*target), *valueExpr, ifExpr, isRecursive);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ForStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

namespace
{
// Jinja2's parse_assign_target, see StatementsParser::ParseAssignTarget
struct AssignTargetParser
{
    LexScanner& lexer;

    nonstd::expected<AssignTarget, ParseError> Parse(bool withNamespace, bool inParens)
    {
        std::vector<AssignTarget> items;
        bool hasComma = false;
        // `()` is an empty tuple
        if (inParens && lexer.PeekNextToken() == ')')
        {
            AssignTarget result;
            result.isTuple = true;
            return result;
        }
        for (;;)
        {
            auto tok = lexer.PeekNextToken();
            nonstd::expected<AssignTarget, ParseError> item;
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
            items.push_back(std::move(*item));

            if (!lexer.EatIfEqual(','))
            {
                break;
            }
            hasComma = true;
        }

        if (!hasComma)
        {
            return std::move(items.front());
        }
        AssignTarget result;
        result.isTuple = true;
        result.items = std::move(items);
        return result;
    }

    // `(...)`: a nested target
    nonstd::expected<AssignTarget, ParseError> ParseParenthesized()
    {
        lexer.NextToken();
        auto inner = Parse(false, true);
        if (!inner)
        {
            return inner;
        }
        if (!lexer.EatIfEqual(')'))
        {
            return MakeParseError(ErrorCode::ExpectedRoundBracket, lexer.PeekNextToken());
        }
        return inner;
    }

    // `name`, or `name.attr` when namespace attributes are allowed
    nonstd::expected<AssignTarget, ParseError> ParseName(const Token& tok, bool withNamespace)
    {
        lexer.NextToken();
        AssignTarget item;
        item.name = AsString(tok.value);
        if (withNamespace && lexer.EatIfEqual('.'))
        {
            auto attrTok = lexer.NextToken();
            if (attrTok != Token::Identifier)
            {
                return MakeParseError(ErrorCode::ExpectedIdentifier, attrTok);
            }
            item.attr = AsString(attrTok.value);
        }
        return item;
    }
};
} // namespace

// Jinja2's parse_assign_target: a name, or names and parenthesised targets separated by
// commas (`a, (b, c)`); for `set` the names outside parentheses can also be namespace
// attributes (`ns.attr`)
nonstd::expected<AssignTarget, ParseError> StatementsParser::ParseAssignTarget(LexScanner& lexer, bool withNamespace)
{
    return AssignTargetParser{ lexer }.Parse(withNamespace, false);
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

    statementsInfo.back().currentComposition->AddRenderer(std::make_shared<LoopControlStatement>(control));
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndFor(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    StatementInfo info = statementsInfo.back();
    RendererPtr elseRenderer;
    if (info.type == StatementInfo::ElseIfStatement)
    {
        auto r = std::static_pointer_cast<ElseBranchStatement>(info.renderer);
        r->SetMainBody(info.compositions[0]);
        elseRenderer = std::static_pointer_cast<IRendererBase>(r);

        statementsInfo.pop_back();
        info = statementsInfo.back();
    }

    if (info.type != StatementInfo::ForStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto* renderer = static_cast<ForStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);
    if (elseRenderer)
    {
        renderer->SetElseBody(elseRenderer);
    }

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseIf(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    const auto& pivotTok = lexer.PeekNextToken();
    ExpressionParser exprParser(m_settings, m_env);
    auto valueExpr = exprParser.ParseTupleOrExpression(lexer);
    if (!valueExpr)
    {
        return MakeParseError(ErrorCode::ExpectedExpression, pivotTok);
    }

    auto renderer = std::make_shared<IfStatement>(*valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::IfStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

namespace
{

// Jinja2: `else` ends an `if`, `elif` or `for` body, and nothing may follow it but the end tag
bool IsElseBranch(const StatementInfo& info)
{
    return info.type == StatementInfo::ElseIfStatement && std::static_pointer_cast<ElseBranchStatement>(info.renderer)->IsElse();
}

} // namespace

StatementsParser::ParseResult StatementsParser::ParseElse(LexScanner& /*lexer*/, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto& prev = statementsInfo.back();
    if ((prev.type != StatementInfo::IfStatement && prev.type != StatementInfo::ElseIfStatement && prev.type != StatementInfo::ForStatement) || IsElseBranch(prev))
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto renderer = std::make_shared<ElseBranchStatement>(ExpressionEvaluatorPtr<>());
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, stmtTok);
    statementInfo.renderer = std::static_pointer_cast<IRendererBase>(renderer);
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseElIf(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto& prev = statementsInfo.back();
    if ((prev.type != StatementInfo::IfStatement && prev.type != StatementInfo::ElseIfStatement) || IsElseBranch(prev))
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    const auto& pivotTok = lexer.PeekNextToken();
    ExpressionParser exprParser(m_settings, m_env);
    auto valueExpr = exprParser.ParseTupleOrExpression(lexer);
    if (!valueExpr)
    {
        return MakeParseError(ErrorCode::ExpectedExpression, pivotTok);
    }

    auto renderer = std::make_shared<ElseBranchStatement>(*valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, stmtTok);
    statementInfo.renderer = std::static_pointer_cast<IRendererBase>(renderer);
    statementsInfo.push_back(std::move(statementInfo));
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndIf(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto info = std::move(statementsInfo.back());
    statementsInfo.pop_back();

    std::list<StatementPtr<ElseBranchStatement>> elseBranches;

    auto errorTok = stmtTok;
    while (info.type != StatementInfo::IfStatement)
    {
        if (info.type != StatementInfo::ElseIfStatement)
        {
            return MakeParseError(ErrorCode::UnexpectedStatement, errorTok);
        }

        auto elseRenderer = std::static_pointer_cast<ElseBranchStatement>(info.renderer);
        elseRenderer->SetMainBody(info.compositions[0]);

        elseBranches.push_front(elseRenderer);
        errorTok = info.token;
        info = std::move(statementsInfo.back());
        statementsInfo.pop_back();
    }

    auto* renderer = static_cast<IfStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);

    for (auto& b : elseBranches)
    {
        renderer->AddElseBranch(b);
    }

    statementsInfo.back().currentComposition->AddRenderer(std::move(info.renderer));

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseSet(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto target = ParseAssignTarget(lexer, true);
    if (!target)
    {
        return MakeUnexpected(target.error());
    }
    auto vars = std::move(*target);

    ExpressionParser exprParser(m_settings, m_env);
    if (lexer.EatIfEqual('='))
    {
        const auto expr = exprParser.ParseTupleOrExpression(lexer);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        statementsInfo.back().currentComposition->AddRenderer(
            std::make_shared<SetLineStatement>(std::move(vars), *expr));
    }
    else if (lexer.EatIfEqual('|'))
    {
        const auto expr = exprParser.ParseFilterExpression(lexer);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        auto statementInfo = StatementInfo::Create(
            StatementInfo::SetStatement, stmtTok);
        statementInfo.renderer = std::make_shared<SetFilteredBlockStatement>(
            std::move(vars), *expr);
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
        statementInfo.renderer = std::make_shared<SetRawBlockStatement>(
            std::move(vars));
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

    const auto info = statementsInfo.back();
    if (info.type != StatementInfo::SetStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto& renderer = *boost::polymorphic_downcast<SetBlockStatement*>(
        info.renderer.get());
    renderer.SetBody(info.compositions[0]);

    statementsInfo.pop_back();
    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

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

    std::string blockName = AsString(nextTok.value);

    // Jinja2 accepts `scoped`, then `required`, in this order
    bool isScoped = lexer.EatIfEqual(Keyword::Scoped);
    bool isRequired = false;
    Token modifierTok = lexer.PeekNextToken();
    if (modifierTok == Token::Identifier && AsString(modifierTok.value) == "required")
    {
        lexer.EatToken();
        isRequired = true;
    }
    modifierTok = lexer.PeekNextToken();
    if (modifierTok != Token::Eof)
    {
        return MakeParseErrorTL(ErrorCode::ExpectedToken, modifierTok, Token::Eof);
    }

    auto blockRenderer = std::make_shared<BlockStatement>(blockName, isScoped, isRequired);
    auto* templateRoot = statementsInfo.front().templateRoot;
    if (templateRoot && !templateRoot->AddBlock(blockRenderer))
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::BlockStatement, stmtTok);
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

    auto info = statementsInfo.back();
    if (info.type != StatementInfo::BlockStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    auto blockStmt = std::static_pointer_cast<BlockStatement>(info.renderer);
    // `endblock` may repeat the name of the block it ends, and only that name
    Token nextTok = lexer.PeekNextToken();
    if (nextTok == Token::Identifier && AsString(nextTok.value) == blockStmt->GetName())
    {
        lexer.EatToken();
    }

    statementsInfo.pop_back();
    blockStmt->SetMainBody(info.compositions[0]);
    statementsInfo.back().currentComposition->AddRenderer(blockStmt);

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

    ExpressionParser exprParser(m_settings, m_env);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }

    auto renderer = std::make_shared<ExtendsStatement>(*expr);
    statementsInfo.back().currentComposition->AddRenderer(renderer);
    if (auto* templateRoot = statementsInfo.front().templateRoot)
    {
        templateRoot->SetHasExtends();
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

    std::string macroName = AsString(nextTok.value);
    MacroParams macroParams;

    if (lexer.EatIfEqual('('))
    {
        auto result = ParseMacroParams(lexer);
        if (!result)
        {
            return MakeUnexpected(result.error());
        }

        macroParams = std::move(result.value());
    }
    else if (lexer.PeekNextToken() != Token::Eof)
    {
        const Token& tok = lexer.PeekNextToken();

        return MakeParseErrorTL(ErrorCode::UnexpectedToken, tok, Token::RBracket, Token::Eof);
    }

    auto renderer = std::make_shared<MacroStatement>(std::move(macroName), std::move(macroParams));
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::MacroStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(std::move(statementInfo));

    return ParseResult();
}

namespace
{
using MacroDefaultTokens = std::pair<Lexer::TokensList::const_iterator, Lexer::TokensList::const_iterator>;

// Does a default name an argument of this macro or a special one (an attribute `x.a` does not count)?
void MarkDefaultsReferringToArgs(MacroParams& items, const std::vector<MacroDefaultTokens>& defaultTokens)
{
    auto isArgName = [&items](const std::string& name) {
        if (name == "caller" || name == "varargs" || name == "kwargs")
        {
            return true;
        }
        return std::any_of(items.begin(), items.end(), [&name](const MacroParam& p) { return p.paramName == name; });
    };
    for (std::size_t idx = 0; idx < items.size(); ++idx)
    {
        const auto& range = defaultTokens[idx];
        for (auto t = range.first; t != range.second && !items[idx].defaultRefersToArgs; ++t)
        {
            bool isAttribute = t != range.first && *std::prev(t) == '.';
            if (t->type == Token::Identifier && !isAttribute && isArgName(AsString(t->value)))
            {
                items[idx].defaultRefersToArgs = true;
            }
        }
    }
}
} // namespace

nonstd::expected<MacroParams, ParseError> StatementsParser::ParseMacroParams(LexScanner& lexer)
{
    MacroParams items;

    if (lexer.EatIfEqual(')'))
    {
        return items;
    }

    std::vector<MacroDefaultTokens> defaultTokens;

    ExpressionParser exprParser(m_settings, m_env);
    do
    {
        Token name = lexer.NextToken();
        if (name != Token::Identifier)
        {
            return MakeParseError(ErrorCode::ExpectedIdentifier, name);
        }

        auto paramName = AsString(name.value);
        auto isSameName = [&paramName](const MacroParam& p) { return p.paramName == paramName; };
        if (std::any_of(items.begin(), items.end(), isSameName))
        {
            return MakeParseError(ErrorCode::UnexpectedToken, name);
        }

        ExpressionEvaluatorPtr<> defVal;
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

        MacroParam p;
        p.paramName = std::move(paramName);
        p.defaultValue = std::move(defVal);
        items.push_back(std::move(p));

    } while (lexer.EatIfEqual(','));

    auto tok = lexer.NextToken();
    if (tok != ')')
    {
        return MakeParseError(ErrorCode::ExpectedRoundBracket, tok);
    }

    MarkDefaultsReferringToArgs(items, defaultTokens);

    return items;
}

StatementsParser::ParseResult StatementsParser::ParseEndMacro(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    StatementInfo info = statementsInfo.back();

    if (info.type != StatementInfo::MacroStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto* renderer = static_cast<MacroStatement*>(info.renderer.get());
    // Jinja2: the special "caller" argument must be omitted or be given a default
    if (renderer->HasInvalidCallerParam())
    {
        return MakeParseError(ErrorCode::UnexpectedToken, info.token);
    }
    renderer->SetMainBody(info.compositions[0]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseCall(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    MacroParams callbackParams;

    if (lexer.EatIfEqual('('))
    {
        auto result = ParseMacroParams(lexer);
        if (!result)
        {
            return MakeUnexpected(result.error());
        }

        callbackParams = std::move(result.value());
    }

    Token nextTok = lexer.NextToken();
    if (nextTok != Token::Identifier)
    {
        const Token& tok = nextTok;
        Token tok1;
        tok1.type = Token::Identifier;

        return MakeParseError(ErrorCode::UnexpectedToken, tok, { tok1 });
    }

    std::string macroName = AsString(nextTok.value);

    CallParamsInfo callParams;
    if (lexer.EatIfEqual('('))
    {
        ExpressionParser exprParser(m_settings, m_env);
        auto result = exprParser.ParseCallParams(lexer);
        if (!result)
        {
            return MakeUnexpected(result.error());
        }

        callParams = std::move(result.value());
    }

    auto renderer = std::make_shared<MacroCallStatement>(std::move(macroName), std::move(callParams), std::move(callbackParams));
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::MacroCallStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(std::move(statementInfo));

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndCall(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    StatementInfo info = statementsInfo.back();

    if (info.type != StatementInfo::MacroCallStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto* renderer = static_cast<MacroCallStatement*>(info.renderer.get());
    // Jinja2: the special "caller" argument must be omitted or be given a default
    if (renderer->HasInvalidCallerParam())
    {
        return MakeParseError(ErrorCode::UnexpectedToken, info.token);
    }
    renderer->SetMainBody(info.compositions[0]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseInclude(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    // auto operTok = lexer.NextToken();
    ExpressionEvaluatorPtr<> valueExpr;
    ExpressionParser exprParser(m_settings, m_env);
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

    auto renderer = std::make_shared<IncludeStatement>(isIgnoreMissing, isWithContext);
    renderer->SetIncludeNamesExpr(valueExpr);
    statementsInfo.back().currentComposition->AddRenderer(renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseImport(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (!m_env)
    {
        return MakeParseError(ErrorCode::TemplateEnvAbsent, stmtTok);
    }

    ExpressionEvaluatorPtr<> valueExpr;
    ExpressionParser exprParser(m_settings, m_env);
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

    auto renderer = std::make_shared<ImportStatement>(isWithContext);
    renderer->SetImportNameExpr(valueExpr);
    renderer->SetNamespace(AsString(name.value));
    statementsInfo.back().currentComposition->AddRenderer(renderer);

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

    macroMap.first = AsString(nextTok.value);
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
        macroMap.second = AsString(nextTok.value);
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

    ExpressionEvaluatorPtr<> valueExpr;
    ExpressionParser exprParser(m_settings, m_env);
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

    auto renderer = std::make_shared<ImportStatement>(isWithContext);
    renderer->SetImportNameExpr(valueExpr);

    for (auto& nameInfo : mappedNames)
    {
        renderer->AddNameToImport(std::move(nameInfo.first), std::move(nameInfo.second));
    }

    statementsInfo.back().currentComposition->AddRenderer(renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseDo(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& /*stmtTok*/)
{
    ExpressionEvaluatorPtr<> valueExpr;
    ExpressionParser exprParser(m_settings, m_env);
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    valueExpr = *expr;

    auto renderer = std::make_shared<DoStatement>(valueExpr);
    statementsInfo.back().currentComposition->AddRenderer(renderer);

    return jinja2::StatementsParser::ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseWith(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> vars;

    ExpressionParser exprParser(m_settings, m_env);
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

        vars.emplace_back(AsString(nameTok.value), valueExpr);

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

    auto renderer = std::make_shared<WithStatement>();
    renderer->SetScopeVars(std::move(vars));
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::WithStatement, stmtTok);
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

    StatementInfo info = statementsInfo.back();

    if (info.type != StatementInfo::WithStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto* renderer = static_cast<WithStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseFilter(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    ExpressionParser exprParser(m_settings, m_env);
    auto filterExpr = exprParser.ParseFilterExpression(lexer);
    if (!filterExpr)
    {
        return MakeUnexpected(filterExpr.error());
    }

    auto renderer = std::make_shared<FilterStatement>(*filterExpr);
    auto statementInfo = StatementInfo::Create(
        StatementInfo::FilterStatement, stmtTok);
    statementInfo.renderer = std::move(renderer);
    statementsInfo.push_back(std::move(statementInfo));

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseEndFilter(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    const auto info = statementsInfo.back();
    if (info.type != StatementInfo::FilterStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto& renderer = *boost::polymorphic_downcast<FilterStatement*>(info.renderer.get());
    renderer.SetBody(info.compositions[0]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseAutoescape(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    ExpressionParser exprParser(m_settings);
    auto valueExpr = exprParser.ParseFullExpression(lexer);
    if (!valueExpr)
    {
        return MakeUnexpected(valueExpr.error());
    }

    auto renderer = std::make_shared<AutoescapeStatement>(*valueExpr);
    auto statementInfo = StatementInfo::Create(StatementInfo::AutoescapeStatement, stmtTok);
    statementInfo.renderer = std::move(renderer);
    statementsInfo.push_back(std::move(statementInfo));

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseEndAutoescape(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    const auto info = statementsInfo.back();
    if (info.type != StatementInfo::AutoescapeStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto& renderer = *boost::polymorphic_downcast<AutoescapeStatement*>(info.renderer.get());
    renderer.SetBody(info.compositions[0]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return {};
}

StatementsParser::ParseResult StatementsParser::ParseTrans(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    auto trans = std::make_shared<TransInfo>();
    Token contextTok;
    if (lexer.EatIfEqual(Token::String, &contextTok))
    {
        trans->context = contextTok.value;
    }

    // The parameters, as Jinja2's InternationalizationExtension.parse reads them
    ExpressionParser exprParser(m_settings, m_env);
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
        auto name = AsString(nameTok.value);
        // Jinja2: translatable variable defined twice
        if (trans->HasVariable(name))
        {
            return MakeParseError(ErrorCode::UnexpectedToken, nameTok);
        }

        ExpressionEvaluatorPtr<> value;
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
            value = std::make_shared<ValueRefExpression>(name);
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
    auto name = stmtTok == Token::Identifier ? AsString(stmtTok.value) : std::string();
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
        auto name = AsString(nameTok.value);
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
void AddTransMessageNames(TransInfo& trans)
{
    for (auto* names : { &trans.singularNames, &trans.pluralNames })
    {
        for (auto& name : *names)
        {
            if (!trans.HasVariable(name))
            {
                trans.variables.emplace_back(name, std::make_shared<ValueRefExpression>(name));
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
    StatementInfo info = statementsInfo.back();
    statementsInfo.pop_back();
    auto& trans = *info.trans;

    // Jinja2: pluralize without variables
    if (trans.hasPlural && trans.pluralVar.empty())
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, info.token);
    }

    AddTransMessageNames(trans);

    if (trans.trimmed.value_or(false))
    {
        TrimTransMessages(trans);
    }

    // gettext(singular, **variables), with the `n` and `p` variants Jinja2 uses for a plural form
    // and a message context; ngettext takes `num` from the count unless the count is `num` itself
    CallParamsInfo params;
    if (!trans.context.IsUndefined())
    {
        params.posParams.push_back(std::make_shared<ConstantExpression>(trans.context));
    }
    params.posParams.push_back(std::make_shared<ConstantExpression>(InternalValue(trans.singular)));
    std::string fnName = trans.context.IsUndefined() ? "gettext" : "pgettext";
    for (size_t idx = 0; idx < trans.variables.size(); ++idx)
    {
        auto& name = trans.variables[idx].first;
        auto slot = std::make_shared<ValueRefExpression>(TransStatement::VariableSlot(idx));
        if (trans.hasPlural && name == trans.pluralVar)
        {
            params.posParams.push_back(std::make_shared<ConstantExpression>(InternalValue(trans.plural)));
            params.posParams.push_back(slot);
            fnName = trans.context.IsUndefined() ? "ngettext" : "npgettext";
            if (name == "num")
            {
                continue;
            }
        }
        params.kwParams[name] = slot;
    }

    ExpressionParser exprParser(m_settings, m_env);
    auto call = std::make_shared<CallExpression>(std::make_shared<ValueRefExpression>(fnName), std::move(params));
    auto output = std::make_shared<ExpressionRenderer>(call, exprParser.GetFinalize());
    statementsInfo.back().currentComposition->AddRenderer(std::make_shared<TransStatement>(std::move(trans.variables), std::move(output)));
    return {};
}

} // namespace jinja2
