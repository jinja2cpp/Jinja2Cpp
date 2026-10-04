#include "expression_parser.h"

#include "error_handling.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "lexer.h"
#include "make_unexpected.h"
#include "recursion_guard.h"
#include "renderer.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template_env.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{

template<typename T>
auto ReplaceErrorIfPossible(T& result, const Token& pivotTok, ErrorCode newError)
{
    auto& error = result.error();
    // Past the nesting limit the next token is fine; the template is too deep
    if (error.errorCode != ErrorCode::RecursionLimitExceeded && error.errorToken.range.startOffset == pivotTok.range.startOffset)
    {
        return MakeParseError(newError, pivotTok);
    }

    return MakeUnexpected(result.error());
}

// Python concatenates adjacent string literals: 'a' 'b' is 'ab'
InternalValue ParseAdjacentStrings(LexScanner& lexer, InternalValue value)
{
    Token tok;
    while (lexer.EatIfEqual(Token::String, &tok))
    {
        auto* str = GetIf<TargetString>(&value);
        auto* next = GetIf<TargetString>(&tok.value);
        if (!str || !next)
        {
            break;
        }

        if (auto* narrow = std::get_if<std::string>(str))
        {
            *narrow += std::get<std::string>(*next);
        }
        else
        {
            std::get<std::wstring>(*str) += std::get<std::wstring>(*next);
        }
    }

    return value;
}

ExpressionParser::ExpressionParser(const Settings& settings, TemplateEnv* env)
    : m_env(env)
{
    if (settings.finalize.callable)
    {
        m_finalize = visitors::InputValueConvertor::ConvertUserCallable(settings.finalize);
    }
}

// Templates bind the filters and tests of the environment when they are loaded, as Jinja2 does
InternalValue ExpressionParser::FindRegisteredFilter(const std::string& name) const
{
    auto filter = m_env ? m_env->FindFilter(name) : std::optional<UserCallable>();
    return filter ? visitors::InputValueConvertor::ConvertUserCallable(*filter) : InternalValue();
}

InternalValue ExpressionParser::FindRegisteredTester(const std::string& name) const
{
    auto tester = m_env ? m_env->FindTest(name) : std::optional<UserCallable>();
    return tester ? visitors::InputValueConvertor::ConvertUserCallable(*tester) : InternalValue();
}

ExpressionParser::ParseResult<RendererPtr> ExpressionParser::Parse(LexScanner& lexer)
{
    auto evaluator = ParseTupleOrExpression(lexer);
    if (!evaluator)
    {
        return MakeUnexpected(evaluator.error());
    }

    const auto& tok = lexer.NextToken();
    if (tok != Token::Eof)
    {
        auto tok1 = tok;
        tok1.type = Token::Eof;

        return MakeParseError(ErrorCode::ExpectedToken, tok, { tok1 });
    }

    // {{ x }} without an inline `if` renders the inner expression directly, a virtual
    // call less per output
    ExpressionEvaluatorPtr<> expr = *evaluator;
    if (const auto* full = dynamic_cast<const FullExpressionEvaluator*>(expr.get()))
    {
        if (auto plain = full->GetPlainExpressionPtr())
        {
            expr = std::move(plain);
        }
    }
    RendererPtr result = std::make_shared<ExpressionRenderer>(std::move(expr), m_finalize);

    return result;
}

bool ExpressionParser::AddOperator()
{
    return ++m_operators <= MaxExpressionOperators && !StackNearlyExhausted();
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<FullExpressionEvaluator>> ExpressionParser::ParseFullExpression(LexScanner& lexer, bool includeIfPart)
{
    // Every nested expression (brackets, call arguments, subscripts, filter arguments, the
    // else branch of a conditional) starts here
    DepthGuard depthGuard(m_depth);
    if (depthGuard.Exceeds(MaxExpressionDepth) || StackNearlyExhausted())
    {
        return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
    }
    ExpressionEvaluatorPtr<FullExpressionEvaluator> result;
    LexScanner::StateSaver saver(lexer);

    ExpressionEvaluatorPtr<FullExpressionEvaluator> evaluator = std::make_shared<FullExpressionEvaluator>();
    auto value = ParseLogicalOr(lexer);
    if (!value)
    {
        return MakeUnexpected(value.error());
    }

    evaluator->SetExpression(*value);

    if (includeIfPart && lexer.EatIfEqual(Keyword::If))
    {
        auto ifExpr = ParseIfExpression(lexer);
        if (!ifExpr)
        {
            return MakeUnexpected(ifExpr.error());
        }
        evaluator->SetTester(*ifExpr);
    }

    saver.Commit();

    return evaluator;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseTupleOrExpression(LexScanner& lexer, bool includeIfPart)
{
    SiblingOperators siblings(m_operators);
    auto first = ParseFullExpression(lexer, includeIfPart);
    if (!first)
    {
        return MakeUnexpected(first.error());
    }
    if (lexer.PeekNextToken() != ',')
    {
        return ExpressionEvaluatorPtr<Expression>(std::move(*first));
    }

    std::vector<ExpressionEvaluatorPtr<>> exprs{ *first };
    while (lexer.EatIfEqual(','))
    {
        // A trailing comma ends the tuple: 'a,' is a one-element tuple
        const auto& next = lexer.PeekNextToken();
        if (next == Token::Eof || next == ')' || next == ']' || next == '}' || next.keyword == Keyword::If || next.keyword == Keyword::Recursive)
        {
            break;
        }
        siblings.Next();
        auto expr = ParseFullExpression(lexer, includeIfPart);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        exprs.push_back(*expr);
    }

    return std::make_shared<TupleCreator>(std::move(exprs), true);
}

// The grammar follows jinja2/parser.py, loosest binding first:
// or, and, not, comparisons (chained, 'in', 'not in'), + -, ~, * / // %, **, unary + -,
// then postfix (attribute, subscript, slice, call), then filters and 'is' tests
ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseLogicalOr(LexScanner& lexer)
{
    OperatorChain chain(m_operators);
    auto left = ParseLogicalAnd(lexer);
    while (left && lexer.EatIfEqual(Keyword::LogicalOr))
    {
        chain.BeforeRight();
        auto right = ParseLogicalAnd(lexer);
        // Every path assigns `left`, the one value returned, so the result is never moved out
        if (!right)
        {
            left = std::move(right);
        }
        else if (!chain.AfterRight(MaxExpressionOperators))
        {
            left = MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
        }
        else
        {
            left = ExpressionEvaluatorPtr<Expression>(std::make_shared<BinaryExpression>(BinaryExpression::LogicalOr, *left, *right));
        }
    }

    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseLogicalAnd(LexScanner& lexer)
{
    OperatorChain chain(m_operators);
    auto left = ParseLogicalNot(lexer);
    while (left && lexer.EatIfEqual(Keyword::LogicalAnd))
    {
        chain.BeforeRight();
        auto right = ParseLogicalNot(lexer);
        // Every path assigns `left`, the one value returned, so the result is never moved out
        if (!right)
        {
            left = std::move(right);
        }
        else if (!chain.AfterRight(MaxExpressionOperators))
        {
            left = MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
        }
        else
        {
            left = ExpressionEvaluatorPtr<Expression>(std::make_shared<BinaryExpression>(BinaryExpression::LogicalAnd, *left, *right));
        }
    }

    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseLogicalNot(LexScanner& lexer)
{
    // 'not a == b' is 'not (a == b)'
    if (!lexer.EatIfEqual(Keyword::LogicalNot))
    {
        return ParseLogicalCompare(lexer);
    }

    if (!AddOperator())
    {
        return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
    }
    auto expr = ParseLogicalNot(lexer);
    if (!expr)
    {
        return expr;
    }

    return std::make_shared<UnaryExpression>(UnaryExpression::LogicalNot, *expr);
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseLogicalCompare(LexScanner& lexer)
{
    auto left = ParseMathPlusMinus(lexer);
    if (!left)
    {
        return left;
    }

    CompareExpression::Operands operands;
    for (;;)
    {
        const auto& tok = lexer.NextToken();
        CompareExpression::Operand operand;
        bool isComparison = true;
        switch (tok.type)
        {
        case Token::Equal:
            operand.operation = BinaryExpression::LogicalEq;
            break;
        case Token::NotEqual:
            operand.operation = BinaryExpression::LogicalNe;
            break;
        case '<':
            operand.operation = BinaryExpression::LogicalLt;
            break;
        case '>':
            operand.operation = BinaryExpression::LogicalGt;
            break;
        case Token::GreaterEqual:
            operand.operation = BinaryExpression::LogicalGe;
            break;
        case Token::LessEqual:
            operand.operation = BinaryExpression::LogicalLe;
            break;
        default:
            if (tok.keyword == Keyword::In)
            {
                operand.operation = BinaryExpression::In;
                break;
            }
            if (tok.keyword == Keyword::LogicalNot && lexer.PeekNextToken().keyword == Keyword::In)
            {
                lexer.EatToken();
                operand.operation = BinaryExpression::In;
                operand.negated = true;
                break;
            }
            lexer.ReturnToken();
            isComparison = false;
            break;
        }
        if (!isComparison)
        {
            break;
        }

        auto right = ParseMathPlusMinus(lexer);
        if (!right)
        {
            left = std::move(right);
            return left;
        }
        operand.expr = *right;
        operands.push_back(std::move(operand));
    }

    if (operands.empty())
    {
        return left;
    }

    // Every path returns `left`, so the result is never moved out
    if (operands.size() > 1)
    {
        left = std::make_shared<CompareExpression>(*left, std::move(operands));
        return left;
    }

    // A single comparison keeps the plain binary node
    auto& operand = operands.front();
    ExpressionEvaluatorPtr<Expression> result = std::make_shared<BinaryExpression>(operand.operation, *left, operand.expr);
    if (operand.negated)
    {
        result = std::make_shared<UnaryExpression>(UnaryExpression::LogicalNot, result);
    }
    left = std::move(result);
    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseMathPlusMinus(LexScanner& lexer)
{
    OperatorChain chain(m_operators);
    auto res = ParseStringConcat(lexer);
    if (!res)
    {
        return res;
    }

    while (true)
    {
        const auto& tok = lexer.NextToken();
        BinaryExpression::Operation operation{};
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
            return res;
        }
        chain.BeforeRight();
        auto right = ParseStringConcat(lexer);
        // Every path returns `res`, so the result is never moved out
        if (!right)
        {
            res = std::move(right);
            return res;
        }
        if (!chain.AfterRight(MaxExpressionOperators))
        {
            res = MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
            return res;
        }
        res = ExpressionEvaluatorPtr<Expression>(std::make_shared<BinaryExpression>(operation, *res, *right));
    }
    return res;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseStringConcat(LexScanner& lexer)
{
    // '~' binds tighter than '+' and looser than '*', as in Jinja2
    OperatorChain chain(m_operators);
    auto left = ParseMathMulDiv(lexer);
    while (left && lexer.EatIfEqual('~'))
    {
        chain.BeforeRight();
        auto right = ParseMathMulDiv(lexer);
        // Every path assigns `left`, the one value returned, so the result is never moved out
        if (!right)
        {
            left = std::move(right);
        }
        else if (!chain.AfterRight(MaxExpressionOperators))
        {
            left = MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
        }
        else
        {
            left = ExpressionEvaluatorPtr<Expression>(std::make_shared<BinaryExpression>(BinaryExpression::StringConcat, *left, *right));
        }
    }
    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseMathMulDiv(LexScanner& lexer)
{
    OperatorChain chain(m_operators);
    auto res = ParseMathPow(lexer);
    if (!res)
    {
        return res;
    }

    while (true)
    {
        const auto& tok = lexer.NextToken();
        BinaryExpression::Operation operation{};
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
            operation = BinaryExpression::DivRemainder;
            break;
        default:
            lexer.ReturnToken();
            return res;
        }
        chain.BeforeRight();
        auto right = ParseMathPow(lexer);
        // Every path returns `res`, so the result is never moved out
        if (!right)
        {
            res = std::move(right);
            return res;
        }
        if (!chain.AfterRight(MaxExpressionOperators))
        {
            res = MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
            return res;
        }
        res = ExpressionEvaluatorPtr<Expression>(std::make_shared<BinaryExpression>(operation, *res, *right));
    }

    return res;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseMathPow(LexScanner& lexer)
{
    // Unlike Python, Jinja2's ** is left-associative and binds tighter than unary minus:
    // 2 ** 3 ** 2 is 64 and -2 ** 2 is 4
    OperatorChain chain(m_operators);
    auto left = ParseUnaryPlusMinus(lexer);
    while (left && lexer.EatIfEqual(Token::MulMul))
    {
        chain.BeforeRight();
        auto right = ParseUnaryPlusMinus(lexer);
        // Every path assigns `left`, the one value returned, so the result is never moved out
        if (!right)
        {
            left = std::move(right);
        }
        else if (!chain.AfterRight(MaxExpressionOperators))
        {
            left = MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
        }
        else
        {
            left = ExpressionEvaluatorPtr<Expression>(std::make_shared<BinaryExpression>(BinaryExpression::Pow, *left, *right));
        }
    }

    return left;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseUnaryPlusMinus(LexScanner& lexer, bool withFilter)
{
    const auto& tok = lexer.NextToken();
    ParseResult<ExpressionEvaluatorPtr<Expression>> result;
    if (tok == '+' || tok == '-')
    {
        if (!AddOperator())
        {
            result = MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
            return result;
        }
        // Filters after a unary operand apply to the negated value: -x|abs is (-x)|abs
        auto subExpr = ParseUnaryPlusMinus(lexer, false);
        if (!subExpr)
        {
            result = std::move(subExpr);
            return result;
        }
        result = ExpressionEvaluatorPtr<Expression>(
            std::make_shared<UnaryExpression>(tok == '+' ? UnaryExpression::UnaryPlus : UnaryExpression::UnaryMinus, *subExpr));
    }
    else
    {
        lexer.ReturnToken();
        result = ParseValueExpression(lexer);
        if (!result)
        {
            return result;
        }
    }

    // Most operands have neither a postfix nor a filter: skip those parsers then
    const auto& next = lexer.PeekNextToken();
    if (next == '.' || next == '[' || next == '(')
    {
        result = ParsePostfix(lexer, std::move(*result));
    }
    const auto& filterStart = lexer.PeekNextToken();
    if (result && withFilter && (filterStart == '|' || filterStart == '(' || filterStart.keyword == Keyword::Is))
    {
        result = ParseFiltersAndTests(lexer, std::move(*result));
    }

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseValueExpression(LexScanner& lexer)
{
    const auto& tok = lexer.NextToken();

    switch (tok.type)
    {
    case Token::Identifier:
    {
        auto kwType = tok.keyword;
        if (kwType == Keyword::Is || kwType == Keyword::In || kwType == Keyword::If || kwType == Keyword::Else)
        {
            return MakeParseError(ErrorCode::UnexpectedToken, tok);
        }

        return std::make_shared<ValueRefExpression>(AsString(tok.value));
    }
    case Token::IntegerNum:
    case Token::FloatNum:
        return std::make_shared<ConstantExpression>(tok.value);
    case Token::String:
        return std::make_shared<ConstantExpression>(ParseAdjacentStrings(lexer, tok.value));
    case Token::True:
        return std::make_shared<ConstantExpression>(InternalValue(true));
    case Token::False:
        return std::make_shared<ConstantExpression>(InternalValue(false));
    case Token::None:
        return std::make_shared<ConstantExpression>(InternalValue(EmptyValue()));
    case '(':
        return ParseBracedExpressionOrTuple(lexer);
    case '[':
        return ParseTuple(lexer);
    case '{':
        return ParseDictionary(lexer);
    default:
        return MakeParseError(ErrorCode::UnexpectedToken, tok);
    }
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParsePostfix(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef)
{
    ParseResult<ExpressionEvaluatorPtr<Expression>> result = std::move(valueRef);
    while (result)
    {
        const auto& tok = lexer.PeekNextToken();
        if ((tok == '.' || tok == '[' || tok == '(') && !AddOperator())
        {
            return MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
        }
        if (tok == '.' || tok == '[')
        {
            result = ParseSubscript(lexer, *result);
        }
        else if (lexer.EatIfEqual('('))
        {
            result = ParseCall(lexer, *result);
        }
        else
        {
            break;
        }
    }

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseFiltersAndTests(LexScanner& lexer,
                                                                                                         ExpressionEvaluatorPtr<Expression> valueRef)
{
    ParseResult<ExpressionEvaluatorPtr<Expression>> result = std::move(valueRef);
    while (result)
    {
        const auto& opTok = lexer.PeekNextToken();
        if (lexer.EatIfEqual('|'))
        {
            if (!AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, opTok);
            }
            auto filter = ParseFilterExpression(lexer);
            if (!filter)
            {
                return MakeUnexpected(filter.error());
            }
            result = ExpressionEvaluatorPtr<Expression>(std::make_shared<FilteredExpression>(std::move(*result), *filter));
        }
        else if (lexer.EatIfEqual(Keyword::Is))
        {
            if (!AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, opTok);
            }
            result = ParseTest(lexer, *result);
        }
        else if (lexer.EatIfEqual('('))
        {
            if (!AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, opTok);
            }
            result = ParseCall(lexer, *result);
        }
        else
        {
            break;
        }
    }

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseTest(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef)
{
    const bool negated = lexer.EatIfEqual(Keyword::LogicalNot);

    // none, true and false are keywords to the lexer but test names to Jinja2
    Token nameTok = lexer.NextToken();
    std::string name;
    if (nameTok == Token::Identifier)
    {
        name = AsString(nameTok.value);
    }
    else if (nameTok == Token::True || nameTok == Token::False || nameTok == Token::None)
    {
        name = lexer.GetAsString(nameTok);
    }
    else
    {
        return MakeParseError(ErrorCode::ExpectedIdentifier, nameTok);
    }

    CallParamsInfo params;
    const auto& argTok = lexer.PeekNextToken();
    if (lexer.EatIfEqual('('))
    {
        auto parsedParams = ParseCallParams(lexer);
        if (!parsedParams)
        {
            return MakeUnexpected(parsedParams.error());
        }
        params = std::move(*parsedParams);
    }
    else
    {
        // A single argument without parentheses: 'is divisibleby 3', 'is sameas none'
        const auto kw = argTok.keyword;
        bool startsArg = false;
        switch (argTok.type)
        {
        case Token::Identifier:
            startsArg = kw != Keyword::Else && kw != Keyword::LogicalOr && kw != Keyword::LogicalAnd;
            break;
        case Token::String:
        case Token::IntegerNum:
        case Token::FloatNum:
        case Token::True:
        case Token::False:
        case Token::None:
        case '[':
        case '{':
            startsArg = true;
            break;
        default:
            break;
        }

        if (startsArg)
        {
            if (kw == Keyword::Is)
            {
                return MakeParseError(ErrorCode::UnexpectedToken, argTok);
            }
            auto arg = ParseValueExpression(lexer);
            if (arg)
            {
                arg = ParsePostfix(lexer, *arg);
            }
            if (!arg)
            {
                return arg;
            }
            params.posParams.push_back(*arg);
        }
    }

    ExpressionEvaluatorPtr<Expression> result;
    try
    {
        result = std::make_shared<IsExpression>(std::move(valueRef), name, std::move(params), FindRegisteredTester(name));
    }
    catch (const std::runtime_error&)
    {
        return MakeParseError(ErrorCode::UnexpectedException, nameTok);
    }

    if (negated)
    {
        result = std::make_shared<UnaryExpression>(UnaryExpression::LogicalNot, result);
    }

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseBracedExpressionOrTuple(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    bool isTuple = false;
    std::vector<ExpressionEvaluatorPtr<Expression>> exprs;
    if (lexer.EatIfEqual(')'))
    {
        return std::make_shared<TupleCreator>(std::move(exprs), true);
    }

    SiblingOperators siblings(m_operators);
    for (;;)
    {
        siblings.Next();
        const auto& pivotTok = lexer.PeekNextToken();
        auto expr = ParseFullExpression(lexer);

        if (!expr)
        {
            return ReplaceErrorIfPossible(expr, pivotTok, ErrorCode::ExpectedRoundBracket);
        }

        exprs.push_back(*expr);
        Token tok = lexer.NextToken();
        if (tok == ')')
        {
            break;
        }
        if (tok != ',')
        {
            return MakeParseError(ErrorCode::ExpectedRoundBracket, tok);
        }

        isTuple = true;
        if (lexer.EatIfEqual(')'))
        {
            break;
        }
    }

    if (isTuple)
    {
        result = std::make_shared<TupleCreator>(std::move(exprs), true);
    }
    else
    {
        result = exprs[0];
    }

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseDictionary(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    DictCreator::Items items;
    if (lexer.EatIfEqual('}'))
    {
        return std::make_shared<DictCreator>(std::move(items));
    }

    SiblingOperators siblings(m_operators);
    do
    {
        // Python's {key: value}, plus the {'key' = value} form Jinja2C++ has always accepted
        siblings.Next();
        auto keyTok = lexer.PeekNextToken();
        auto key = ParseFullExpression(lexer);
        if (!key)
        {
            return ReplaceErrorIfPossible(key, keyTok, ErrorCode::ExpectedExpression);
        }

        auto sepTok = lexer.NextToken();
        if (sepTok != Token::Colon && (sepTok != '=' || keyTok != Token::String))
        {
            auto tok1 = sepTok;
            tok1.type = Token::Colon;
            return MakeParseError(ErrorCode::ExpectedToken, sepTok, { tok1 });
        }

        siblings.Next();
        const auto& pivotTok = lexer.PeekNextToken();
        auto expr = ParseFullExpression(lexer);
        if (!expr)
        {
            return ReplaceErrorIfPossible(expr, pivotTok, ErrorCode::ExpectedExpression);
        }

        items.emplace_back(*key, *expr);

    } while (lexer.EatIfEqual(',') && lexer.PeekNextToken() != '}');

    const auto& tok = lexer.NextToken();
    if (tok != '}')
    {
        return MakeParseError(ErrorCode::ExpectedCurlyBracket, tok);
    }

    result = std::make_shared<DictCreator>(std::move(items));

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseTuple(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<Expression> result;

    std::vector<ExpressionEvaluatorPtr<Expression>> exprs;
    if (lexer.EatIfEqual(']'))
    {
        return std::make_shared<TupleCreator>(exprs);
    }

    SiblingOperators siblings(m_operators);
    do
    {
        siblings.Next();
        auto expr = ParseFullExpression(lexer);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }

        exprs.push_back(*expr);
    } while (lexer.EatIfEqual(',') && lexer.PeekNextToken() != ']');

    const auto& tok = lexer.NextToken();
    if (tok != ']')
    {
        return MakeParseError(ErrorCode::ExpectedSquareBracket, tok);
    }

    result = std::make_shared<TupleCreator>(std::move(exprs));

    return result;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseCall(LexScanner& lexer, const ExpressionEvaluatorPtr<Expression>& valueRef)
{
    ExpressionEvaluatorPtr<Expression> result;

    ParseResult<CallParamsInfo> params = ParseCallParams(lexer);
    if (!params)
    {
        return MakeUnexpected(params.error());
    }

    result = std::make_shared<CallExpression>(valueRef, std::move(*params));

    return result;
}

ExpressionParser::ParseResult<CallParamsInfo> ExpressionParser::ParseCallParams(LexScanner& lexer)
{
    CallParamsInfo result;

    if (lexer.EatIfEqual(')'))
    {
        return result;
    }

    SiblingOperators siblings(m_operators);
    do
    {
        siblings.Next();
        const auto& tok = lexer.NextToken();
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
            return MakeUnexpected(valueExpr.error());
        }
        if (paramName.empty())
        {
            result.posParams.push_back(*valueExpr);
        }
        else
        {
            result.kwParams[paramName] = *valueExpr;
        }

        // A trailing comma ends the arguments: f(a, ) is f(a)
    } while (lexer.EatIfEqual(',') && lexer.PeekNextToken() != ')');

    const auto& tok = lexer.NextToken();
    if (tok != ')')
    {
        return MakeParseError(ErrorCode::ExpectedRoundBracket, tok);
    }

    return result;
}

namespace
{
template<typename T>
using SubscriptParseResult = nonstd::expected<T, ParseError>;

// The index of l.attr: a name, or an integer constant (l.0 is l[0])
struct DotSubscript
{
    ExpressionEvaluatorPtr<Expression> indexExpr;
    std::string attrName;
};

SubscriptParseResult<DotSubscript> ParseDotSubscript(LexScanner& lexer)
{
    DotSubscript result;
    // l.0 is l[0]; any other attribute is looked up by name
    const auto& tok = lexer.NextToken();
    if (tok == Token::Identifier)
    {
        result.attrName = AsString(tok.value);
    }
    else if (tok == Token::True || tok == Token::False || tok == Token::None)
    {
        result.attrName = lexer.GetAsString(tok);
    }
    else if ((tok == Token::IntegerNum || tok == Token::FloatNum) && GetIf<int64_t>(&tok.value))
    {
        result.indexExpr = std::make_shared<ConstantExpression>(tok.value);
    }
    else
    {
        return MakeParseError(ErrorCode::ExpectedIdentifier, tok);
    }

    return result;
}

// The parts of l[start:stop:step]; only parts[0] is set for a plain index
struct BracketSubscript
{
    ExpressionEvaluatorPtr<> parts[3];
    bool isSlice = false;
};

bool EndsSlicePart(LexScanner& lexer)
{
    const auto& next = lexer.PeekNextToken();
    return next == ']' || next == ':' || next == ',';
}

// One expression of the subscript, a sibling of the others
SubscriptParseResult<ExpressionEvaluatorPtr<>> ParseSubscriptPart(ExpressionParser& parser, LexScanner& lexer, SiblingOperators& siblings)
{
    siblings.Next();
    auto expr = parser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    return ExpressionEvaluatorPtr<>(*expr);
}

// l[a, b] and l[] index with a tuple, as in Jinja2; first is the already parsed a, if any
SubscriptParseResult<ExpressionEvaluatorPtr<>> ParseSubscriptTuple(ExpressionParser& parser,
                                                                   LexScanner& lexer,
                                                                   SiblingOperators& siblings,
                                                                   const ExpressionEvaluatorPtr<>& first)
{
    std::vector<ExpressionEvaluatorPtr<>> items;
    if (first)
    {
        items.push_back(first);
    }
    while (lexer.EatIfEqual(',') && lexer.PeekNextToken() != ']')
    {
        auto expr = ParseSubscriptPart(parser, lexer, siblings);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        items.push_back(*expr);
    }
    return ExpressionEvaluatorPtr<>(std::make_shared<TupleCreator>(std::move(items), true));
}

// The ':stop:step' tail of a slice, after its first ':'
nonstd::expected<void, ParseError> ParseSliceTail(ExpressionParser& parser, LexScanner& lexer, SiblingOperators& siblings, BracketSubscript& result)
{
    result.isSlice = true;
    if (!EndsSlicePart(lexer))
    {
        auto expr = ParseSubscriptPart(parser, lexer, siblings);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        result.parts[1] = *expr;
    }
    if (lexer.EatIfEqual(':') && !EndsSlicePart(lexer))
    {
        auto expr = ParseSubscriptPart(parser, lexer, siblings);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        result.parts[2] = *expr;
    }
    return {};
}

// Jinja2's parse_subscribed: an index, or a slice [start:stop:step] with any part omitted
SubscriptParseResult<BracketSubscript> ParseBracketSubscript(ExpressionParser& parser, LexScanner& lexer, SiblingOperators& siblings)
{
    BracketSubscript result;
    auto& start = result.parts[0];
    if (!EndsSlicePart(lexer))
    {
        auto expr = ParseSubscriptPart(parser, lexer, siblings);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        start = *expr;
    }
    // l[a, b] and l[] index with a tuple, as in Jinja2
    if (lexer.PeekNextToken() != ':' && (lexer.PeekNextToken() == ',' || (!start && lexer.PeekNextToken() == ']')))
    {
        auto tuple = ParseSubscriptTuple(parser, lexer, siblings, start);
        if (!tuple)
        {
            return MakeUnexpected(tuple.error());
        }
        start = *tuple;
    }
    if (lexer.EatIfEqual(':'))
    {
        auto tail = ParseSliceTail(parser, lexer, siblings, result);
        if (!tail)
        {
            return MakeUnexpected(tail.error());
        }
    }
    else if (!start)
    {
        return MakeParseError(ErrorCode::ExpectedExpression, lexer.PeekNextToken());
    }

    if (!lexer.EatIfEqual(']'))
    {
        return MakeParseError(ErrorCode::ExpectedSquareBracket, lexer.PeekNextToken());
    }

    return result;
}
} // namespace

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<Expression>> ExpressionParser::ParseSubscript(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef)
{
    ExpressionEvaluatorPtr<Expression> indexExpr;
    std::string attrName;
    if (lexer.NextToken() == '.')
    {
        auto dot = ParseDotSubscript(lexer);
        if (!dot)
        {
            return MakeUnexpected(dot.error());
        }
        indexExpr = std::move(dot->indexExpr);
        attrName = std::move(dot->attrName);
    }
    else
    {
        SiblingOperators siblings(m_operators);
        auto bracket = ParseBracketSubscript(*this, lexer, siblings);
        if (!bracket)
        {
            return MakeUnexpected(bracket.error());
        }
        auto& parts = bracket->parts;
        if (bracket->isSlice)
        {
            return std::make_shared<SliceExpression>(std::move(valueRef), parts[0], parts[1], parts[2]);
        }
        indexExpr = parts[0];
    }

    // Consecutive subscripts share one node: a.b[0].c
    auto subscript = std::dynamic_pointer_cast<SubscriptExpression>(valueRef);
    if (!subscript)
    {
        subscript = std::make_shared<SubscriptExpression>(std::move(valueRef));
    }
    subscript->AddIndex(std::move(indexExpr), std::move(attrName));

    return subscript;
}

ExpressionParser::ParseResult<ExpressionEvaluatorPtr<ExpressionFilter>> ExpressionParser::ParseFilterExpression(LexScanner& lexer)
{
    ExpressionEvaluatorPtr<ExpressionFilter> result;

    const auto& startTok = lexer.PeekNextToken();
    try
    {
        do
        {
            const auto& tok = lexer.NextToken();
            if (tok != Token::Identifier)
            {
                return MakeParseError(ErrorCode::ExpectedIdentifier, tok);
            }

            std::string name = AsString(tok.value);
            ParseResult<CallParamsInfo> params;

            if (lexer.NextToken() == '(')
            {
                params = ParseCallParams(lexer);
            }
            else
            {
                lexer.ReturnToken();
            }

            if (!params)
            {
                return MakeUnexpected(params.error());
            }

            auto filter = std::make_shared<ExpressionFilter>(name, std::move(*params), FindRegisteredFilter(name));
            if (result && !AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
            }
            if (result)
            {
                filter->SetParentFilter(result);
                result = filter;
            }
            else
            {
                result = filter;
            }

        } while (lexer.NextToken() == '|');

        lexer.ReturnToken();
    }
    catch (const ParseError& error)
    {
        return MakeUnexpected(error);
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

    try
    {
        auto testExpr = ParseLogicalOr(lexer);
        if (!testExpr)
        {
            return MakeUnexpected(testExpr.error());
        }

        ParseResult<ExpressionEvaluatorPtr<>> altValue;
        if (lexer.PeekNextToken().keyword == Keyword::Else)
        {
            lexer.EatToken();
            // 'a if x else b if y else c' chains like an operator; Python reads it in a loop
            if (!AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
            }
            --m_depth;
            auto value = ParseFullExpression(lexer);
            ++m_depth;
            if (!value)
            {
                return MakeUnexpected(value.error());
            }
            altValue = *value;
        }

        result = std::make_shared<IfExpression>(*testExpr, *altValue);
    }
    catch (const ParseError& error)
    {
        return MakeUnexpected(error);
    }
    catch (const std::runtime_error& ex)
    {
        std::cout << "Filter parsing problem: " << ex.what() << '\n';
    }

    return result;
}

} // namespace jinja2
