#include "expression_parser.h"

#include <sstream>

namespace jinja2
{

template<typename T>
auto ReplaceErrorIfPossible(T& result, const Token& pivotTok, ErrorCode newError)
{
    auto& error = result.error();
    if (error.errorToken.range.startOffset == pivotTok.range.startOffset)
        return MakeParseError(newError, pivotTok);

    return result.get_unexpected();
}

ExpressionParser::ExpressionParser()
{

}

ExpressionParser::ParseResult<RendererPtr> ExpressionParser::Parse(LexScanner& lexer)
{
    auto evaluator = ParseFullExpression(lexer);
    if (!evaluator)
        return evaluator.get_unexpected();

    RendererPtr result = std::make_shared<ExpressionRenderer>(*evaluator);

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<FullExpressionEvaluator>> ExpressionParser::ParseFullExpression(LexScanner &lexer, bool includeIfPart)
{
    ExpressionEvaluatorPtr<FullExpressionEvaluator> result;
    LexScanner::StateSaver saver(lexer);

    ExpressionEvaluatorPtr<FullExpressionEvaluator> evaluator = std::make_shared<FullExpressionEvaluator>();
    auto value = ParseLogicalOr(lexer);
    if (!value)
        return value.get_unexpected();

    evaluator->SetExpression(*value);
    if (lexer.EatIfEqual('|'))
    {
        auto filter = ParseFilterExpression(lexer);
        if (!filter)
            return filter.get_unexpected();
        evaluator->SetFilter(*filter);
    }

    if (includeIfPart && lexer.EatIfEqual(Token::If))
    {
        if (includeIfPart)
        {
            lexer.EatToken();
            auto ifExpr = ParseIfExpression(lexer);
            if (!ifExpr)
                return ifExpr.get_unexpected();
            evaluator->SetTester(*ifExpr);
        }
    }

    saver.Commit();

    return evaluator;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseLogicalOr(LexScanner& lexer)
{
    auto left = ParseLogicalAnd(lexer);

    if (left && lexer.EatIfEqual(Token::LogicalOr))
    {
        auto right = ParseLogicalOr(lexer);
        if (!right)
            return right.get_unexpected();

        return std::make_shared<BinaryExpression>(BinaryExpression::LogicalOr, *left, *right);
    }

    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseLogicalAnd(LexScanner& lexer)
{
    auto left = ParseLogicalCompare(lexer);

    if (left && lexer.EatIfEqual(Token::LogicalAnd))
    {
        auto right = ParseLogicalAnd(lexer);
        if (!right)
            return right;

        return std::make_shared<BinaryExpression>(BinaryExpression::LogicalAnd, *left, *right);
    }

    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseLogicalCompare(LexScanner& lexer)
{
    auto left = ParseStringConcat(lexer);
    if (!left)
        return left;

    auto tok = lexer.NextToken();
    BinaryExpression::Operation operation;
    switch (tok.type)
    {
    case Token::Equal:
        operation = BinaryExpression::LogicalEq;
        break;
    case Token::NotEqual:
        operation = BinaryExpression::LogicalNe;
        break;
    case '<':
        operation = BinaryExpression::LogicalLt;
        break;
    case '>':
        operation = BinaryExpression::LogicalGt;
        break;
    case Token::GreaterEqual:
        operation = BinaryExpression::LogicalGe;
        break;
    case Token::LessEqual:
        operation = BinaryExpression::LogicalLe;
        break;
    case Token::In:
        operation = BinaryExpression::In;
        break;
    case Token::Is:
    {
        Token nextTok = lexer.NextToken();
        if (nextTok != Token::Identifier)
            return ExpressionEvaluatorPtr<Expression>();

        std::string name = AsString(nextTok.value);
        ParseResult<CallParams> params;

        if (lexer.EatIfEqual('('))
            params = ParseCallParams(lexer);

        if (!params)
            return params.get_unexpected();

        return std::make_shared<IsExpression>(*left, std::move(name), std::move(*params));
    }
    default:
        lexer.ReturnToken();
        return left;
    }

    auto right = ParseStringConcat(lexer);
    if (!right)
        return right;

    return std::make_shared<BinaryExpression>(operation, *left, *right);
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseStringConcat(LexScanner& lexer)
{
    auto left = ParseMathPow(lexer);

    if (left && lexer.EatIfEqual('~'))
    {
        auto right = ParseLogicalAnd(lexer);
        if (!right)
            return right;

        return std::make_shared<BinaryExpression>(BinaryExpression::StringConcat, *left, *right);
    }
    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseMathPow(LexScanner& lexer)
{
    auto left = ParseMathPlusMinus(lexer);

    if (left && lexer.EatIfEqual(Token::MulMul))
    {
        auto right = ParseMathPow(lexer);
        if (!right)
            return ExpressionEvaluatorPtr<Expression>();

        return std::make_shared<BinaryExpression>(BinaryExpression::Pow, *left, *right);
    }

    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseMathPlusMinus(LexScanner& lexer)
{
    auto left = ParseMathMulDiv(lexer);
    if (!left)
        return left;

    auto tok = lexer.NextToken();
    BinaryExpression::Operation operation;
    switch (tok.type)
    {
    case '+':
        operation = BinaryExpression::Plus;
        break;
    case '-':
        operation = BinaryExpression::Minus;
        break;
    default:
        lexer.ReturnToken();
        return left;
    }

    auto right = ParseMathPlusMinus(lexer);
    if (!right)
        return right;

    return std::make_shared<BinaryExpression>(operation, *left, *right);
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseMathMulDiv(LexScanner& lexer)
{
    auto left = ParseUnaryPlusMinus(lexer);
    if (!left)
        return left;

    auto tok = lexer.NextToken();
    BinaryExpression::Operation operation;
    switch (tok.type)
    {
    case '*':
        operation = BinaryExpression::Mul;
        break;
    case '/':
        operation = BinaryExpression::Div;
        break;
    case Token::DivDiv:
        operation = BinaryExpression::DivInteger;
        break;
    case '%':
        operation = BinaryExpression::DivReminder;
        break;
    default:
        lexer.ReturnToken();
        return left;
    }

    auto right = ParseMathMulDiv(lexer);
    if (!right)
        return right;

    return std::make_shared<BinaryExpression>(operation, *left, *right);
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseUnaryPlusMinus(LexScanner& lexer)
{
    Token tok = lexer.NextToken();
    if (tok != '+' && tok != '-' && tok != Token::LogicalNot)
    {
        lexer.ReturnToken();
        return ParseValueExpression(lexer);
    }

    auto subExpr = ParseValueExpression(lexer);
    if (!subExpr)
        return subExpr;

    return std::make_shared<UnaryExpression>(tok == '+' ? UnaryExpression::UnaryPlus : (tok == '-' ? UnaryExpression::UnaryMinus : UnaryExpression::LogicalNot), *subExpr);
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseValueExpression(LexScanner& lexer)
{
    Token tok = lexer.NextToken();

    ParseResult<ExpressionEvaluatorPtr<Expression>> valueRef;

    switch (tok.type)
    {
    case Token::Identifier:
        valueRef = std::make_shared<ValueRefExpression>(AsString(tok.value));
        break;
    case Token::IntegerNum:
    case Token::FloatNum:
    case Token::String:
        return std::make_shared<ConstantExpression>(tok.value);
    case Token::True:
        return std::make_shared<ConstantExpression>(InternalValue(true));
    case Token::False:
        return std::make_shared<ConstantExpression>(InternalValue(false));
    case '(':
        valueRef = ParseBracedExpressionOrTuple(lexer);
        break;
    case '[':
        valueRef = ParseTuple(lexer);
        break;
    case '{':
        valueRef = ParseDictionary(lexer);
        break;
    default:
        return MakeParseError(ErrorCode::UnexpectedToken, tok);
    }

    if (valueRef)
    {
        tok = lexer.PeekNextToken();
        if (tok == '[' || tok == '.')
            valueRef = ParseSubscript(lexer, *valueRef);

        if (lexer.EatIfEqual('('))
            valueRef = ParseCall(lexer, *valueRef);
    }
    return valueRef;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseBracedExpressionOrTuple(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    bool isTuple = false;
    std::vector<ExpressionEvaluatorPtr<Expression>> exprs;
    for (;;)
    {
        Token pivotTok = lexer.PeekNextToken();
        auto expr = ParseFullExpression(lexer);

        if (!expr)
            return ReplaceErrorIfPossible(expr, pivotTok, ErrorCode::ExpectedRoundBracket);

        exprs.push_back(*expr);
        Token tok = lexer.NextToken();
        if (tok == ')')
            break;
        else if (tok == ',')
            isTuple = true;
    }

    if (isTuple)
        result = std::make_shared<TupleCreator>(std::move(exprs));
    else
        result = exprs[0];

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseDictionary(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    std::unordered_map<std::string, ExpressionEvaluatorPtr<Expression>> items;
    while (lexer.NextToken() != '}')
    {
        lexer.ReturnToken();;
        Token key = lexer.NextToken();
        if (key != Token::String)
            return MakeParseError(ErrorCode::ExpectedStringLiteral, key);

        if (!lexer.EatIfEqual('='))
            return MakeParseError(ErrorCode::ExpectedStringLiteral, lexer.NextToken());

        auto expr = ParseFullExpression(lexer);
        if (!expr)
            return expr.get_unexpected();

        items[AsString(key.value)] = *expr;

        lexer.EatIfEqual(',');
    }

    result = std::make_shared<DictCreator>(std::move(items));

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseTuple(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    std::vector<ExpressionEvaluatorPtr<Expression>> exprs;
    while (lexer.NextToken() != ']')
    {
        lexer.ReturnToken();
        auto expr = ParseFullExpression(lexer);
        if (!expr)
            return expr.get_unexpected();

        exprs.push_back(*expr);
        lexer.EatIfEqual(',');
    }

    result = std::make_shared<TupleCreator>(std::move(exprs));

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseCall(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef)
{
    ExpressionEvaluatorPtr<Expression> result;

    ParseResult<CallParams> params = ParseCallParams(lexer);
    if (!params)
        return params.get_unexpected();

    result = std::make_shared<CallExpression>(valueRef, std::move(*params));

    return result;
}

ExpressionParser::ParseResult<CallParams> ExpressionParser::ParseCallParams(LexScanner& lexer)
{
    CallParams result;

    while (lexer.NextToken() != ')')
    {
        lexer.ReturnToken();
        Token tok = lexer.NextToken();
        std::string paramName;
        if (tok == Token::Identifier && lexer.PeekNextToken() == '=')
        {
            paramName = AsString(tok.value);
            lexer.EatToken();
        }
        else
        {
            lexer.ReturnToken();
        }

        auto valueExpr = ParseFullExpression(lexer);
        if (!valueExpr)
        {
            return valueExpr.get_unexpected();
        }
        if (paramName.empty())
            result.posParams.push_back(*valueExpr);
        else
            result.kwParams[paramName] = *valueExpr;

        lexer.EatIfEqual(',');
    }

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseSubscript(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef)
{
    for (Token tok = lexer.NextToken(); tok.type == '.' || tok.type == '['; tok = lexer.NextToken())
    {
        ParseResult<ExpressionEvaluatorPtr<Expression>> indexExpr;
        if (tok == '.')
        {
            tok = lexer.NextToken();
            if (tok.type != Token::Identifier)
                return MakeParseError(ErrorCode::ExpectedIdentifier, tok);

            auto valueName = AsString(tok.value);
            indexExpr = std::make_shared<ConstantExpression>(InternalValue(valueName));
        }
        else
        {
            auto expr = ParseFullExpression(lexer);

            if (!indexExpr)
                return indexExpr.get_unexpected();
            else
                indexExpr = *expr;

            if (!lexer.EatIfEqual(']', &tok))
                return MakeParseError(ErrorCode::ExpectedSquareBracket, tok);
        }

        valueRef = std::make_shared<SubscriptExpression>(valueRef, *indexExpr);
    }

    lexer.ReturnToken();

    return valueRef;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<ExpressionFilter>> ExpressionParser::ParseFilterExpression(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<ExpressionFilter> result;

    auto startTok = lexer.PeekNextToken();
    try
    {
        do
        {
            Token tok = lexer.NextToken();
            if (tok != Token::Identifier)
                return MakeParseError(ErrorCode::ExpectedIdentifier, tok);

            std::string name = AsString(tok.value);
            ParseResult<CallParams> params;

            if (lexer.NextToken() == '(')
                params = ParseCallParams(lexer);
            else
                lexer.ReturnToken();

            if (!params)
                return params.get_unexpected();

            auto filter = std::make_shared<ExpressionFilter>(name, std::move(*params));
            if (result)
            {
                filter->SetParentFilter(result);
                result = filter;
            }
            else
                result = filter;

        } while (lexer.NextToken() == '|');

        lexer.ReturnToken();
    }
    catch (const ParseError& error)
    {
        return nonstd::make_unexpected(error);
    }
    catch (const std::runtime_error&)
    {
        return MakeParseError(ErrorCode::UnexpectedException, startTok);
    }
    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<IfExpression>> ExpressionParser::ParseIfExpression(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<IfExpression> result;

    auto startTok = lexer.PeekNextToken();
    try
    {
        auto testExpr = ParseLogicalOr(lexer);
        if (!testExpr)
            return testExpr.get_unexpected();

        ParseResult<ExpressionEvaluatorPtr<>> altValue;
        if (lexer.PeekNextToken() == Token::Else)
        {
            lexer.EatToken();
            auto value = ParseFullExpression(lexer);
            if (!value)
                return value.get_unexpected();
            altValue = *value;
        }

        result = std::make_shared<IfExpression>(*testExpr, *altValue);
    }
    catch (const ParseError& error)
    {
        return nonstd::make_unexpected(error);
    }
    catch (const std::runtime_error& ex)
    {
        std::cout << "Filter parsing problem: " << ex.what() << std::endl;
    }

    return result;
}

} // jinja2
