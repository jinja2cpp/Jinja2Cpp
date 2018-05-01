#include "expression_parser.h"

#include <sstream>

namespace jinja2
{

ExpressionParser::ExpressionParser()
{

}

RendererPtr ExpressionParser::Parse(LexScanner& lexer)
{
    auto evaluator = ParseFullExpression(lexer);
    if (!evaluator)
        return RendererPtr();

    auto result = std::make_shared<ExpressionRenderer>(evaluator);

    return result;
}

ExpressionEvaluatorPtr<FullExpressionEvaluator> ExpressionParser::ParseFullExpression(LexScanner &lexer)
{
    ExpressionEvaluatorPtr<FullExpressionEvaluator> result;
    LexScanner::StateSaver saver(lexer);

    ExpressionEvaluatorPtr<FullExpressionEvaluator> evaluator = std::make_shared<FullExpressionEvaluator>();
    auto value = ParseLogicalOr(lexer);
    if (!value)
        return result;

    evaluator->SetExpression(value);
    ExpressionEvaluatorPtr<ExpressionFilter> filter;
    if (lexer.PeekNextToken() == '|')
    {
        lexer.EatToken();
        filter = ParseFilterExpression(lexer);
        if (!filter)
            return result;
        evaluator->SetFilter(filter);
    }

    ExpressionEvaluatorPtr<IfExpression> ifExpr;
    if (lexer.PeekNextToken() == Token::If)
    {
        lexer.EatToken();
        ifExpr = ParseIfExpression(lexer);
        if (!ifExpr)
            return result;
        evaluator->SetTester(ifExpr);
    }

    saver.Commit();

    return evaluator;
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseLogicalOr(LexScanner& lexer)
{
    auto left = ParseLogicalAnd(lexer);
    if (!left)
        return ExpressionEvaluatorPtr<Expression>();

    if (lexer.NextToken() != Token::LogicalOr)
    {
        lexer.ReturnToken();
        return left;
    }
    auto right = ParseLogicalOr(lexer);
    if (!right)
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<BinaryExpression>(BinaryExpression::LogicalOr, left, right);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseLogicalAnd(LexScanner& lexer)
{
    auto left = ParseLogicalCompare(lexer);
    if (!left)
        return ExpressionEvaluatorPtr<Expression>();

    if (lexer.NextToken() != Token::LogicalAnd)
    {
        lexer.ReturnToken();
        return left;
    }
    auto right = ParseLogicalAnd(lexer);
    if (!right)
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<BinaryExpression>(BinaryExpression::LogicalAnd, left, right);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseLogicalCompare(LexScanner& lexer)
{
    auto left = ParseStringConcat(lexer);
    if (!left)
        return ExpressionEvaluatorPtr<Expression>();

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
        Token tok = lexer.NextToken();
        if (tok != Token::Identifier)
            return ExpressionEvaluatorPtr<Expression>();

        std::string name = tok.value.asString();
        bool valid = true;
        CallParams params;

        if (lexer.NextToken() == '(')
            params = ParseCallParams(lexer, valid);

        if (!valid)
            return ExpressionEvaluatorPtr<Expression>();

        return std::make_shared<IsExpression>(left, std::move(name), std::move(params));
    }
    default:
        lexer.ReturnToken();
        return left;
    }

    auto right = ParseStringConcat(lexer);
    if (!right)
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<BinaryExpression>(operation, left, right);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseStringConcat(LexScanner& lexer)
{
    auto left = ParseMathPow(lexer);
    if (!left)
        return ExpressionEvaluatorPtr<Expression>();

    if (lexer.NextToken() != '~')
    {
        lexer.ReturnToken();
        return left;
    }
    auto right = ParseLogicalAnd(lexer);
    if (!right)
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<BinaryExpression>(BinaryExpression::StringConcat, left, right);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseMathPow(LexScanner& lexer)
{
    auto left = ParseMathPlusMinus(lexer);
    if (!left)
        return ExpressionEvaluatorPtr<Expression>();

    if (lexer.NextToken() != Token::MulMul)
    {
        lexer.ReturnToken();
        return left;
    }
    auto right = ParseMathPow(lexer);
    if (!right)
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<BinaryExpression>(BinaryExpression::Pow, left, right);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseMathPlusMinus(LexScanner& lexer)
{
    auto left = ParseMathMulDiv(lexer);
    if (!left)
        return ExpressionEvaluatorPtr<Expression>();

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
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<BinaryExpression>(operation, left, right);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseMathMulDiv(LexScanner& lexer)
{
    auto left = ParseUnaryPlusMinus(lexer);
    if (!left)
        return ExpressionEvaluatorPtr<Expression>();

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
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<BinaryExpression>(operation, left, right);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseUnaryPlusMinus(LexScanner& lexer)
{
    Token tok = lexer.NextToken();
    if (tok != '+' && tok != '-' && tok != Token::LogicalNot)
    {
        lexer.ReturnToken();
        return ParseValueExpression(lexer);
    }

    auto subExpr = ParseValueExpression(lexer);
    if (!subExpr)
        return ExpressionEvaluatorPtr<Expression>();

    return std::make_shared<UnaryExpression>(tok == '+' ? UnaryExpression::UnaryPlus : (tok == '-' ? UnaryExpression::UnaryMinus : UnaryExpression::LogicalNot), subExpr);
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseValueExpression(LexScanner& lexer)
{
    Token tok = lexer.NextToken();

    switch (tok.type)
    {
    case Token::Identifier:
    {
        lexer.ReturnToken();
        auto valueRef = ParseValueRef(lexer);
        if (valueRef.empty())
            return ExpressionEvaluatorPtr<Expression>();

        Token nextTok = lexer.NextToken();
        if (nextTok == '[')
        {
            return ParseSubsicpt(lexer, valueRef);
        }
        else if (nextTok == '(')
        {
            return ParseCall(lexer, valueRef);
        }

        lexer.ReturnToken();

        ExpressionEvaluatorPtr<Expression> baseValueRef = std::make_shared<ValueRefExpression>(valueRef[0]);
        if (valueRef.size() != 1)
        {
            for (size_t i = 1; i < valueRef.size(); ++ i)
            {
                auto indexExpr = std::make_shared<ConstantExpression>(Value(valueRef[i]));
                baseValueRef = std::make_shared<SubscriptExpression>(baseValueRef, indexExpr);
            }
        }
        return baseValueRef;
    }
    case Token::IntegerNum:
    case Token::FloatNum:
    case Token::String:
        return std::make_shared<ConstantExpression>(tok.value);
    case Token::True:
        return std::make_shared<ConstantExpression>(Value(true));
    case Token::False:
        return std::make_shared<ConstantExpression>(Value(false));
    case '(':
        return ParseBracedExpressionOrTuple(lexer);
    case '[':
        return ParseTuple(lexer);
    case '{':
        return ParseDictionary(lexer);
    }

    ExpressionEvaluatorPtr<Expression> result;

    return result;
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseBracedExpressionOrTuple(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    bool isTuple = false;
    std::vector<ExpressionEvaluatorPtr<Expression>> exprs;
    for (;;)
    {
        auto expr = ParseFullExpression(lexer);
        if (!expr)
            return result;

        exprs.push_back(expr);
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

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseDictionary(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    std::unordered_map<std::string, ExpressionEvaluatorPtr<Expression>> items;
    while (lexer.NextToken() != '}')
    {
        lexer.ReturnToken();;
        Token key = lexer.NextToken();
        if (key != Token::String)
            return result;

        if (lexer.NextToken() != '=')
            return result;

        auto expr = ParseFullExpression(lexer);
        if (!expr)
            return result;

        items[key.value.asString()] = expr;

        if (lexer.PeekNextToken() == ',')
            lexer.EatToken();
    }

    result = std::make_shared<DictionaryCreator>(std::move(items));

    return result;
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseTuple(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    std::vector<ExpressionEvaluatorPtr<Expression>> exprs;
    while (lexer.NextToken() != ']')
    {
        lexer.ReturnToken();
        auto expr = ParseFullExpression(lexer);
        if (!expr)
            return result;

        exprs.push_back(expr);
        if (lexer.PeekNextToken() == ',')
            lexer.EatToken();
    }

    result = std::make_shared<TupleCreator>(std::move(exprs));

    return result;
}

ExpressionEvaluatorPtr<SubscriptExpression> ExpressionParser::ParseSubsicpt(LexScanner& lexer, const std::vector<std::string>& valueRef)
{
    ExpressionEvaluatorPtr<SubscriptExpression> result;

    auto finalIndexExpr = ParseFullExpression(lexer);
    if (!finalIndexExpr || lexer.PeekNextToken() != ']')
        return result;

    lexer.EatToken();

    ExpressionEvaluatorPtr<Expression> baseValueRef = std::make_shared<ValueRefExpression>(valueRef[0]);
    if (valueRef.size() != 1)
    {
        for (size_t i = 1; i < valueRef.size(); ++ i)
        {
            auto indexExpr = std::make_shared<ConstantExpression>(Value(valueRef[i]));
            baseValueRef = std::make_shared<SubscriptExpression>(baseValueRef, indexExpr);
        }
    }

    result = std::make_shared<SubscriptExpression>(baseValueRef, finalIndexExpr);

    return result;
}

ExpressionEvaluatorPtr<Expression> ExpressionParser::ParseCall(LexScanner& lexer, const std::vector<std::string>& valueRef)
{
    ExpressionEvaluatorPtr<Expression> result;

    bool isValid = false;
    CallParams params = ParseCallParams(lexer, isValid);
    if (!isValid)
        return result;

    result = std::make_shared<CallExpression>(valueRef, std::move(params));

    return result;
}

CallParams ExpressionParser::ParseCallParams(LexScanner& lexer, bool& isValid)
{
    CallParams result;

    isValid = true;
    while (lexer.NextToken() != ')')
    {
        lexer.ReturnToken();
        Token tok = lexer.NextToken();
        std::string paramName;
        if (tok == Token::Identifier && lexer.PeekNextToken() == '=')
        {
            paramName = tok.value.asString();
            lexer.EatToken();
        }
        else
        {
            lexer.ReturnToken();
        }

        auto valueExpr = ParseFullExpression(lexer);
        if (!valueExpr)
        {
            isValid = false;
            return result;
        }
        if (paramName.empty())
            result.posParams.push_back(valueExpr);
        else
            result.kwParams[paramName] = valueExpr;

        if (lexer.PeekNextToken() == ',')
            lexer.EatToken();
    }

    return result;
}

std::vector<std::string> ExpressionParser::ParseValueRef(LexScanner& lexer)
{
    Token tok = lexer.NextToken();
    auto valueName = tok.value.asString();

    std::vector<std::string> result;
    result.push_back(valueName);

    while (lexer.NextToken() == '.')
    {
        tok = lexer.NextToken();
        if (tok != Token::Identifier)
            return std::vector<std::string>();

        result.push_back(tok.value.asString());
    }
    lexer.ReturnToken();

    return result;
}

ExpressionEvaluatorPtr<ExpressionFilter> ExpressionParser::ParseFilterExpression(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<ExpressionFilter> empty;
    ExpressionEvaluatorPtr<ExpressionFilter> result;

    try
    {
        do
        {
            Token tok = lexer.NextToken();
            if (tok != Token::Identifier)
                return empty;

            std::string name = tok.value.asString();
            bool valid = true;
            CallParams params;

            if (lexer.NextToken() == '(')
                params = ParseCallParams(lexer, valid);

            if (!valid)
                return empty;

            auto filter = std::make_shared<ExpressionFilter>(name, std::move(params));
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
    catch (const std::runtime_error& ex)
    {
        std::cout << "Filter parsing problem: " << ex.what() << std::endl;
    }
    return result;
}

ExpressionEvaluatorPtr<IfExpression> ExpressionParser::ParseIfExpression(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<IfExpression> empty;
    ExpressionEvaluatorPtr<IfExpression> result;

    try
    {
        auto testExpr = ParseLogicalOr(lexer);
        if (!testExpr)
            return empty;

        ExpressionEvaluatorPtr<> altValue;
        if (lexer.PeekNextToken() == Token::Else)
        {
            lexer.EatToken();
            altValue = ParseFullExpression(lexer);
            if (!altValue)
                return empty;
        }

        result = std::make_shared<IfExpression>(testExpr, altValue);
    }
    catch (const std::runtime_error& ex)
    {
        std::cout << "Filter parsing problem: " << ex.what() << std::endl;
    }

    return result;
}

} // jinja2
