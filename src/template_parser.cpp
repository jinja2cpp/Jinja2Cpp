#include "template_parser.h"
#include <iostream>

namespace jinja2
{

StatementsParser::ParseResult StatementsParser::Parse(LexScanner& lexer, StatementInfoList& statementsInfo)
{
    Token tok = lexer.NextToken();
    ParseResult result;

    switch (tok.type)
    {
    case jinja2::Token::For:
        result = ParseFor(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Endfor:
        result = ParseEndFor(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::If:
        result = ParseIf(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Else:
        result = ParseElse(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::ElIf:
        result = ParseElIf(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::EndIf:
        result = ParseEndIf(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Set:
        result = ParseSet(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Block:
        result = ParseBlock(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::EndBlock:
        result = ParseEndBlock(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Extends:
        result = ParseExtends(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Macro:
        result = ParseMacro(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::EndMacro:
        result = ParseEndMacro(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Call:
        result = ParseCall(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::EndCall:
        result = ParseEndCall(lexer, statementsInfo, tok);
        break;
    case jinja2::Token::Filter:
    case jinja2::Token::EndFilter:
    case jinja2::Token::EndSet:
    case jinja2::Token::Include:
    case jinja2::Token::Import:
        return MakeParseError(ErrorCode::YetUnsupported, tok);
    default:
        return MakeParseError(ErrorCode::UnexpectedToken, tok);
    }

    if (result)
    {
        tok = lexer.PeekNextToken();
        if (tok != Token::Eof)
            return MakeParseError(ErrorCode::ExpectedEndOfStatement, tok);
    }

    return result;
}

StatementsParser::ParseResult StatementsParser::ParseFor(LexScanner &lexer, StatementInfoList &statementsInfo,
                                                         const Token &stmtTok)
{
    std::vector<std::string> vars;

    while (lexer.PeekNextToken() == Token::Identifier)
    {
        auto tok = lexer.NextToken();
        vars.push_back(AsString(tok.value));
        if (lexer.NextToken() != ',')
        {
            lexer.ReturnToken();
            break;
        }
    }

    if (vars.empty())
        return MakeParseError(ErrorCode::ExpectedIdentifier, lexer.PeekNextToken());

    if (!lexer.EatIfEqual(Token::In))
    {
        Token tok1 = lexer.PeekNextToken();
        Token tok2 = tok1;
        tok2.type = Token::Identifier;
        tok2.range.endOffset = tok2.range.startOffset;
        tok2.value = InternalValue();
        Token tok3 = tok2;
        tok3.type = Token::In;
        Token tok4 = tok2;
        tok4.type = static_cast<Token::Type>(',');
        return MakeParseError(ErrorCode::ExpectedToken, tok1, {tok2, tok3, tok4});
    }

    auto pivotToken = lexer.PeekNextToken();
    ExpressionParser exprPraser;
    auto valueExpr = exprPraser.ParseFullExpression(lexer, false);
    if (!valueExpr)
        return valueExpr.get_unexpected();
        // return MakeParseError(ErrorCode::ExpectedExpression, pivotToken);

    Token flagsTok;
    bool isRecursive = false;
    if (lexer.EatIfEqual(Token::Identifier, &flagsTok))
    {
        auto flagsName = AsString(flagsTok.value);
        if (flagsName != "recursive")
        {
            auto tok2 = flagsTok;
            tok2.type = Token::Identifier;
            tok2.range.endOffset = tok2.range.startOffset;
            tok2.value = std::string("recursive");
            auto tok3 = flagsTok;
            tok3.type = Token::If;
            return MakeParseError(ErrorCode::ExpectedToken, flagsTok, {tok2, tok3});
        }

        isRecursive = true;
    }

    ExpressionEvaluatorPtr<> ifExpr;
    if (lexer.EatIfEqual(Token::If))
    {
        auto parsedExpr = exprPraser.ParseFullExpression(lexer, false);
        if (!parsedExpr)
            return parsedExpr.get_unexpected();
        ifExpr = *parsedExpr;
    }
    else if (lexer.PeekNextToken() != Token::Eof)
    {
        auto tok1 = lexer.PeekNextToken();
        auto tok2 = tok1;
        tok2.type = Token::If;
        auto tok3 = tok1;
        tok3.type = Token::Eof;
        return MakeParseError(ErrorCode::ExpectedToken, tok1, {tok2, tok3});
    }

    auto renderer = std::make_shared<ForStatement>(vars, *valueExpr, ifExpr, isRecursive);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ForStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndFor(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    StatementInfo info = statementsInfo.back();
    RendererPtr elseRenderer;
    if (info.type == StatementInfo::ElseIfStatement)
    {
        auto r = std::static_pointer_cast<ElseBranchStatement>(info.renderer);
        r->SetMainBody(info.compositions[0]);
        elseRenderer = r;

        statementsInfo.pop_back();
        info = statementsInfo.back();
    }

    if (info.type != StatementInfo::ForStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto renderer = static_cast<ForStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);
    if (elseRenderer)
        renderer->SetElseBody(elseRenderer);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseIf(LexScanner &lexer, StatementInfoList &statementsInfo,
                                                        const Token &stmtTok)
{
    auto pivotTok = lexer.PeekNextToken();
    ExpressionParser exprParser;
    auto valueExpr = exprParser.ParseFullExpression(lexer);
    if (!valueExpr)
        return MakeParseError(ErrorCode::ExpectedExpression, pivotTok);

    auto renderer = std::make_shared<IfStatement>(*valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::IfStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseElse(LexScanner& /*lexer*/, StatementInfoList& statementsInfo
                                                          , const Token& stmtTok)
{
    auto renderer = std::make_shared<ElseBranchStatement>(ExpressionEvaluatorPtr<>());
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseElIf(LexScanner& lexer, StatementInfoList& statementsInfo
                                                          , const Token& stmtTok)
{
    auto pivotTok = lexer.PeekNextToken();
    ExpressionParser exprParser;
    auto valueExpr = exprParser.ParseFullExpression(lexer);
    if (!valueExpr)
        return MakeParseError(ErrorCode::ExpectedExpression, pivotTok);

    auto renderer = std::make_shared<ElseBranchStatement>(*valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndIf(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    auto info = statementsInfo.back();
    statementsInfo.pop_back();

    std::list<StatementPtr<ElseBranchStatement>> elseBranches;

    auto errorTok = stmtTok;
    while (info.type != StatementInfo::IfStatement)
    {
        if (info.type != StatementInfo::ElseIfStatement)
            return MakeParseError(ErrorCode::UnexpectedStatement, errorTok);

        auto elseRenderer = std::static_pointer_cast<ElseBranchStatement>(info.renderer);
        elseRenderer->SetMainBody(info.compositions[0]);

        elseBranches.push_front(elseRenderer);
        errorTok = info.token;
        info = statementsInfo.back();
        statementsInfo.pop_back();
    }

    auto renderer = static_cast<IfStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);

    for (auto& b : elseBranches)
        renderer->AddElseBranch(b);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseSet(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    std::vector<std::string> vars;

    while (lexer.PeekNextToken() == Token::Identifier)
    {
        auto tok = lexer.NextToken();
        vars.push_back(AsString(tok.value));
        if (lexer.NextToken() != ',')
        {
            lexer.ReturnToken();
            break;
        }
    }

    if (vars.empty())
        return MakeParseError(ErrorCode::ExpectedIdentifier, lexer.PeekNextToken());

    auto operTok = lexer.NextToken();
    ExpressionEvaluatorPtr<> valueExpr;
    if (operTok == '=')
    {
        ExpressionParser exprParser;
        auto expr = exprParser.ParseFullExpression(lexer);
        if (!expr)
            return expr.get_unexpected();
        valueExpr = *expr;
    }
    else
        return MakeParseError(ErrorCode::YetUnsupported, operTok, {stmtTok}); // TODO: Add handling of the block assignments

    auto renderer = std::make_shared<SetStatement>(vars);
    renderer->SetAssignmentExpr(valueExpr);
    statementsInfo.back().currentComposition->AddRenderer(renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndSet(LexScanner& /*lexer*/, StatementInfoList& /*statementsInfo*/
                                                            , const Token& stmtTok)
{
    return MakeParseError(ErrorCode::YetUnsupported, stmtTok);
}

StatementsParser::ParseResult StatementsParser::ParseBlock(LexScanner& lexer, StatementInfoList& statementsInfo
                                                           , const Token& stmtTok)
{
    if (statementsInfo.empty())
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    Token nextTok = lexer.NextToken();
    if (nextTok != Token::Identifier)
        return MakeParseError(ErrorCode::ExpectedIdentifier, nextTok);

    std::string blockName = AsString(nextTok.value);

    auto& info = statementsInfo.back();
    RendererPtr blockRenderer;
    StatementInfo::Type blockType = StatementInfo::ParentBlockStatement;
    if (info.type == StatementInfo::ExtendsStatement)
    {
        blockRenderer = std::make_shared<BlockStatement>(blockName);
        blockType = StatementInfo::BlockStatement;
    }
    else
    {
        bool isScoped = false;
        if (lexer.EatIfEqual(Token::Identifier, &nextTok))
        {
            auto id = AsString(nextTok.value);
            if (id != "scoped")
            {
                auto tok2 = nextTok;
                tok2.range.startOffset = tok2.range.endOffset;
                tok2.value = std::string("scoped");
                return MakeParseError(ErrorCode::ExpectedToken, nextTok, {tok2});
            }
            isScoped = true;
        }
        blockRenderer = std::make_shared<ParentBlockStatement>(blockName, isScoped);
    }

    StatementInfo statementInfo = StatementInfo::Create(blockType, stmtTok);
    statementInfo.renderer = blockRenderer;
    statementsInfo.push_back(statementInfo);
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndBlock(LexScanner& lexer, StatementInfoList& statementsInfo
                                                              , const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    Token nextTok = lexer.PeekNextToken();
    if (nextTok != Token::Identifier && nextTok != Token::Eof)
    {
        Token tok2;
        tok2.type = Token::Identifier;
        Token tok3;
        tok3.type = Token::Eof;
        return MakeParseError(ErrorCode::ExpectedToken, nextTok, {tok2, tok3});
    }

    if (nextTok == Token::Identifier)
        lexer.EatToken();

    auto info = statementsInfo.back();
    statementsInfo.pop_back();

    if (info.type == StatementInfo::BlockStatement)
    {
        auto blockStmt = std::static_pointer_cast<BlockStatement>(info.renderer);
        blockStmt->SetMainBody(info.compositions[0]);
        auto& extendsInfo = statementsInfo.back();
        auto extendsStmt = std::static_pointer_cast<ExtendsStatement>(extendsInfo.renderer);
        extendsStmt->AddBlock(std::static_pointer_cast<BlockStatement>(info.renderer));
    }
    else
    {
        auto blockStmt = std::static_pointer_cast<ParentBlockStatement>(info.renderer);
        blockStmt->SetMainBody(info.compositions[0]);
        statementsInfo.back().currentComposition->AddRenderer(info.renderer);
    }

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseExtends(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    Token tok = lexer.NextToken();
    if (tok != Token::String && tok != Token::Identifier)
    {
        auto tok2 = tok;
        tok2.type = Token::Identifier;
        tok2.range.endOffset = tok2.range.startOffset;
        tok2.value = EmptyValue{};
        auto tok3 = tok2;
        tok3.type = Token::String;
        return MakeParseError(ErrorCode::ExpectedToken, tok, {tok2, tok3});
    }

    auto renderer = std::make_shared<ExtendsStatement>(AsString(tok.value), tok == Token::String);
    statementsInfo.back().currentComposition->AddRenderer(renderer);

    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ExtendsStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseMacro(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    Token nextTok = lexer.NextToken();
    if (nextTok != Token::Identifier)
        return MakeParseError(ErrorCode::ExpectedIdentifier, nextTok);

    std::string macroName = AsString(nextTok.value);
    MacroParams macroParams;

    if (lexer.EatIfEqual('('))
    {
        auto result = ParseMacroParams(lexer);
        if (!result)
            return result.get_unexpected();

        macroParams = std::move(result.value());
    }
    else if (lexer.PeekNextToken() != Token::Eof)
    {
        Token tok = lexer.PeekNextToken();
        Token tok1;
        tok1.type = Token::RBracket;
        Token tok2;
        tok2.type = Token::Eof;

        return MakeParseError(ErrorCode::UnexpectedToken, tok, {tok1, tok2});
    }

    auto renderer = std::make_shared<MacroStatement>(std::move(macroName), std::move(macroParams));
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::MacroStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);

    return ParseResult();
}

nonstd::expected<MacroParams, ParseError> StatementsParser::ParseMacroParams(LexScanner& lexer)
{
    MacroParams items;

    if (lexer.EatIfEqual(')'))
        return std::move(items);

    ExpressionParser exprParser;

    do
    {
        Token name = lexer.NextToken();
        if (name != Token::Identifier)
            return MakeParseError(ErrorCode::ExpectedIdentifier, name);

        ExpressionEvaluatorPtr<> defVal;
        if (lexer.EatIfEqual('='))
        {
            auto result = exprParser.ParseFullExpression(lexer, false);
            if (!result)
                return result.get_unexpected();

            defVal = *result;
        }

        MacroParam p;
        p.paramName = AsString(name.value);
        p.defaultValue = defVal;
        items.push_back(std::move(p));

    } while (lexer.EatIfEqual(','));

    auto tok = lexer.NextToken();
    if (tok != ')')
        return MakeParseError(ErrorCode::ExpectedRoundBracket, tok);

    return std::move(items);
}

StatementsParser::ParseResult StatementsParser::ParseEndMacro(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    StatementInfo info = statementsInfo.back();

    if (info.type != StatementInfo::MacroStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto renderer = static_cast<MacroStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseCall(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    MacroParams callbackParams;

    if (lexer.EatIfEqual('('))
    {
        auto result = ParseMacroParams(lexer);
        if (!result)
            return result.get_unexpected();

        callbackParams = std::move(result.value());
    }

    Token nextTok = lexer.NextToken();
    if (nextTok != Token::Identifier)
    {
        Token tok = nextTok;
        Token tok1;
        tok1.type = Token::Identifier;

        return MakeParseError(ErrorCode::UnexpectedToken, tok, {tok1});
    }

    std::string macroName = AsString(nextTok.value);

    CallParams callParams;
    if (lexer.EatIfEqual('('))
    {
        ExpressionParser exprParser;
        auto result = exprParser.ParseCallParams(lexer);
        if (!result)
            return result.get_unexpected();

        callParams = std::move(result.value());
    }

    auto renderer = std::make_shared<MacroCallStatement>(std::move(macroName), std::move(callParams), std::move(callbackParams));
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::MacroCallStatement, stmtTok);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);

    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndCall(LexScanner&, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.size() <= 1)
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    StatementInfo info = statementsInfo.back();

    if (info.type != StatementInfo::MacroCallStatement)
    {
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);
    }

    statementsInfo.pop_back();
    auto renderer = static_cast<MacroCallStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return ParseResult();
}

}
