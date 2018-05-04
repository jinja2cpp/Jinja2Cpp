#include "template_parser.h"
#include <iostream>

namespace jinja2
{

bool StatementsParser::Parse(LexScanner& lexer, StatementInfoList& statementsInfo)
{
    Token tok = lexer.NextToken();
    bool result = true;

    switch (tok.type)
    {
    case jinja2::Token::For:
        result = ParseFor(lexer, statementsInfo, tok.range.startOffset);
        break;
    case jinja2::Token::Endfor:
        result = ParseEndFor(lexer, statementsInfo, tok.range.startOffset);
        break;
    case jinja2::Token::If:
        result = ParseIf(lexer, statementsInfo, tok.range.startOffset);
        break;
    case jinja2::Token::Else:
        result = ParseElse(lexer, statementsInfo, tok.range.startOffset);
        break;
    case jinja2::Token::ElIf:
        result = ParseElIf(lexer, statementsInfo, tok.range.startOffset);
        break;
    case jinja2::Token::EndIf:
        result = ParseEndIf(lexer, statementsInfo, tok.range.startOffset);
        break;
    case jinja2::Token::Set:
        result = ParseSet(lexer, statementsInfo, tok.range.startOffset);
        break;
    case jinja2::Token::Block:
    case jinja2::Token::EndBlock:
    case jinja2::Token::Extends:
    case jinja2::Token::Macro:
    case jinja2::Token::EndMacro:
    case jinja2::Token::Call:
    case jinja2::Token::EndCall:
    case jinja2::Token::Filter:
    case jinja2::Token::EndFilter:
    case jinja2::Token::EndSet:
    case jinja2::Token::Include:
    case jinja2::Token::Import:
        break;
    default:
        return false;
    }

    result &= result && lexer.PeekNextToken() == Token::Eof;

    return result;
}

bool StatementsParser::ParseFor(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos)
{
    std::vector<std::string> vars;

    while (lexer.PeekNextToken() == Token::Identifier)
    {
        auto tok = lexer.NextToken();
        vars.push_back(tok.value.asString());
        if (lexer.NextToken() != ',')
        {
            lexer.ReturnToken();
            break;
        }
    }

    if (vars.empty())
        return false;

    if (lexer.NextToken() != Token::In)
        return false;

    ExpressionParser exprPraser;
    auto valueExpr = exprPraser.ParseFullExpression(lexer);
    if (!valueExpr)
        return false;

    auto renderer = std::make_shared<ForStatement>(vars, valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ForStatement, pos);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return true;
}

bool StatementsParser::ParseEndFor(LexScanner& /*lexer*/, StatementInfoList& statementsInfo, size_t /*pos*/)
{
    if (statementsInfo.empty())
        return false;

    auto info = statementsInfo.back();
    if (info.type != StatementInfo::ForStatement)
        return false;

    statementsInfo.pop_back();
    auto renderer = static_cast<ForStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);
    if (info.compositions.size() == 2)
        renderer->SetElseBody(info.compositions[1]);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return true;
}

bool StatementsParser::ParseIf(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos)
{
    ExpressionParser exprPraser;
    auto valueExpr = exprPraser.ParseFullExpression(lexer);
    if (!valueExpr)
        return false;

    auto renderer = std::make_shared<IfStatement>(valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::IfStatement, pos);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return true;
}

bool StatementsParser::ParseElse(LexScanner& /*lexer*/, StatementInfoList& statementsInfo, size_t pos)
{
    auto renderer = std::make_shared<ElseBranchStatement>(ExpressionEvaluatorPtr<>());
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, pos);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return true;
}

bool StatementsParser::ParseElIf(LexScanner& lexer, StatementInfoList& statementsInfo, size_t pos)
{
    ExpressionParser exprPraser;
    auto valueExpr = exprPraser.ParseFullExpression(lexer);
    if (!valueExpr)
        return false;

    auto renderer = std::make_shared<ElseBranchStatement>(valueExpr);
    StatementInfo statementInfo = StatementInfo::Create(StatementInfo::ElseIfStatement, pos);
    statementInfo.renderer = renderer;
    statementsInfo.push_back(statementInfo);
    return true;
}

bool StatementsParser::ParseEndIf(LexScanner& /*lexer*/, StatementInfoList& statementsInfo, size_t /*pos*/)
{
    if (statementsInfo.empty())
        return false;

    auto info = statementsInfo.back();
    statementsInfo.pop_back();

    std::list<StatementPtr<ElseBranchStatement>> elseBranches;

    while (info.type != StatementInfo::IfStatement)
    {
        if (info.type != StatementInfo::ElseIfStatement)
            return false;

        auto elseRenderer = std::static_pointer_cast<ElseBranchStatement>(info.renderer);
        elseRenderer->SetMainBody(info.compositions[0]);

        elseBranches.push_front(elseRenderer);
        info = statementsInfo.back();
        statementsInfo.pop_back();
    }

    auto renderer = static_cast<IfStatement*>(info.renderer.get());
    renderer->SetMainBody(info.compositions[0]);

    for (auto& b : elseBranches)
        renderer->AddElseBranch(b);

    statementsInfo.back().currentComposition->AddRenderer(info.renderer);

    return true;
}

bool StatementsParser::ParseSet(LexScanner& lexer, StatementInfoList& statementsInfo, size_t /*pos*/)
{
    std::vector<std::string> vars;

    while (lexer.PeekNextToken() == Token::Identifier)
    {
        auto tok = lexer.NextToken();
        vars.push_back(tok.value.asString());
        if (lexer.NextToken() != ',')
        {
            lexer.ReturnToken();
            break;
        }
    }

    if (vars.empty())
        return false;

    auto operTok = lexer.NextToken();
    ExpressionEvaluatorPtr<> valueExpr;
    if (operTok == '=')
    {
        ExpressionParser exprParser;
        valueExpr = exprParser.ParseFullExpression(lexer);
        if (!valueExpr)
            return false;
    }
    else
        return false; // TODO: Add handling of the block assignments

    auto renderer = std::make_shared<SetStatement>(vars);
    if (valueExpr)
    {
        renderer->SetAssignmentExpr(valueExpr);
        statementsInfo.back().currentComposition->AddRenderer(renderer);
    }
    else
        return false;

    return true;
}

bool StatementsParser::ParseEndSet(LexScanner& /*lexer*/, StatementInfoList& /*statementsInfo*/, size_t /*pos*/)
{
    return false;
}

}
