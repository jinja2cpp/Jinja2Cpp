#include "template_parser.h"
#include <iostream>

namespace jinja2
{

StatementsParser::ParseResult StatementsParser::Parse(LexScanner& lexer, StatementInfoList& statementsInfo)
{
    Token tok = lexer.NextToken();
    ParseResult result;

    switch (lexer.GetAsKeyword(tok))
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
    case Keyword::Filter:
    case Keyword::EndFilter:
    case Keyword::EndSet:
    case Keyword::Import:
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

struct ErrorTokenConverter
{
    const Token& baseTok;
    
    ErrorTokenConverter(const Token& t)
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
            newTok.range.endOffset = newTok.range.startOffset;
        return newTok;
    }
};

template<typename ... Args>
auto MakeParseErrorTL(ErrorCode code, const Token& baseTok, Args ...  expectedTokens)
{
    ErrorTokenConverter tokCvt(baseTok);
    
    return MakeParseError(code, baseTok, {tokCvt(expectedTokens)...});
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

    if (!lexer.EatIfEqual(Keyword::In))
    {
        Token tok1 = lexer.PeekNextToken();
        Token tok2 = tok1;
        tok2.type = Token::Identifier;
        tok2.range.endOffset = tok2.range.startOffset;
        tok2.value = InternalValue();
        return MakeParseErrorTL(ErrorCode::ExpectedToken, tok1, tok2, Token::In, ',');
    }

    auto pivotToken = lexer.PeekNextToken();
    ExpressionParser exprPraser;
    auto valueExpr = exprPraser.ParseFullExpression(lexer, false);
    if (!valueExpr)
        return valueExpr.get_unexpected();
        // return MakeParseError(ErrorCode::ExpectedExpression, pivotToken);

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
            return parsedExpr.get_unexpected();
        ifExpr = *parsedExpr;
    }
    else if (lexer.PeekNextToken() != Token::Eof)
    {
        auto tok1 = lexer.PeekNextToken();
        return MakeParseErrorTL(ErrorCode::ExpectedToken, tok1, Token::If, Token::Recursive, Token::Eof);
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
        if (lexer.EatIfEqual(Keyword::Scoped, &nextTok))
            isScoped = true;
        else
        {
            nextTok = lexer.PeekNextToken();
            if (nextTok != Token::Eof)
                return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Scoped);
        }
            
        blockRenderer = std::make_shared<ParentBlockStatement>(blockName, isScoped);
    }

    StatementInfo statementInfo = StatementInfo::Create(blockType, stmtTok);
    statementInfo.renderer = blockRenderer;
    statementsInfo.push_back(statementInfo);
    return ParseResult();
}

StatementsParser::ParseResult StatementsParser::ParseEndBlock(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
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
        return MakeParseErrorTL(ErrorCode::ExpectedToken, tok, tok2, Token::String);
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

        return MakeParseErrorTL(ErrorCode::UnexpectedToken, tok, Token::RBracket, Token::Eof);
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

StatementsParser::ParseResult StatementsParser::ParseInclude(LexScanner& lexer, StatementInfoList& statementsInfo, const Token& stmtTok)
{
    if (statementsInfo.empty())
        return MakeParseError(ErrorCode::UnexpectedStatement, stmtTok);

    // auto operTok = lexer.NextToken();
    ExpressionEvaluatorPtr<> valueExpr;
    ExpressionParser exprParser;
    auto expr = exprParser.ParseFullExpression(lexer);
    if (!expr)
        return expr.get_unexpected();
    valueExpr = *expr;

    Token nextTok;
    bool isIgnoreMissing = false;
    bool isWithContext = true;
    bool hasIgnoreMissing = false;
    if (lexer.EatIfEqual(Keyword::Ignore, &nextTok))
    {
        if (lexer.EatIfEqual(Keyword::Missing, &nextTok))
            isIgnoreMissing = true;
        else
            return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Missing);
            
        hasIgnoreMissing = true;
    }

    nextTok = lexer.NextToken();
    auto kw = lexer.GetAsKeyword(nextTok);
    bool hasContextControl = false;
    if (kw == Keyword::With || kw == Keyword::Without)
    {
        isWithContext = kw == Keyword::With;
        nextTok = lexer.PeekNextToken();
        if (!lexer.EatIfEqual(Keyword::Missing, &nextTok))
            return MakeParseErrorTL(ErrorCode::ExpectedToken, nextTok, Token::Context);
            
        hasContextControl = true;
    }

    if (nextTok != Token::Eof)
    {
        if (hasContextControl)
            return MakeParseErrorTL(ErrorCode::ExpectedEndOfStatement, nextTok, Token::Eof);

        if (hasIgnoreMissing)
            return MakeParseErrorTL(ErrorCode::UnexpectedToken, nextTok, Token::Eof, Token::With, Token::Without);

        return MakeParseErrorTL(ErrorCode::UnexpectedToken, nextTok, Token::Eof, Token::Ignore, Token::With, Token::Without);
    }


    auto renderer = std::make_shared<IncludeStatement>(isIgnoreMissing, isWithContext);
    renderer->SetIncludeNamesExpr(valueExpr);
    statementsInfo.back().currentComposition->AddRenderer(renderer);

    return ParseResult();
}

}
