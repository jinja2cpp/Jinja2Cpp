#include "expression_parser.h"

#include "error_handling.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "lexer.h"
#include "loop_attr.h"
#include "make_unexpected.h"
#include "name_resolver.h"
#include "node_arena.h"
#include "recursion_guard.h"
#include "renderer.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template_env.h>

#include <cstdint>
#include <iostream>
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

ExpressionParser::ExpressionParser(const Settings& settings, TemplateEnv* env, NodeArena& nodes, NameResolver& names)
    : m_env(env)
    , m_nodes(nodes)
    , m_names(names)
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

ExpressionParser::ParseResult<NodeRef<IRendererBase>> ExpressionParser::Parse(LexScanner& lexer)
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

    auto result = MakeExpressionRenderer(m_nodes, *evaluator, m_finalize);

    return result;
}

bool ExpressionParser::AddOperator()
{
    return ++m_operators <= MaxExpressionOperators && !StackNearlyExhausted();
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseFullExpression(LexScanner& lexer, bool includeIfPart)
{
    // Every nested expression (brackets, call arguments, subscripts, filter arguments, the
    // else branch of a conditional) starts here
    DepthGuard depthGuard(m_depth);
    if (depthGuard.Exceeds(MaxExpressionDepth) || StackNearlyExhausted())
    {
        return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
    }
    LexScanner::StateSaver saver(lexer);

    auto value = ParseLogicalOr(lexer);
    if (!value)
    {
        return MakeUnexpected(value.error());
    }

    // Only an inline `if` needs the wrapper: every compound node checks the stack itself
    if (includeIfPart && lexer.EatIfEqual(Keyword::If))
    {
        auto ifExpr = ParseIfExpression(lexer);
        if (!ifExpr)
        {
            return MakeUnexpected(ifExpr.error());
        }
        auto evaluator = m_nodes.Make<FullExpressionEvaluator>();
        m_nodes[evaluator].SetExpression(*value);
        m_nodes[evaluator].SetTester(*ifExpr);
        saver.Commit();
        return NodeRef<Expression>(evaluator);
    }

    saver.Commit();

    return std::move(*value);
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseTupleOrExpression(LexScanner& lexer, bool includeIfPart)
{
    SiblingOperators siblings(m_operators);
    auto first = ParseFullExpression(lexer, includeIfPart);
    if (!first)
    {
        return MakeUnexpected(first.error());
    }
    if (lexer.PeekNextToken() != ',')
    {
        return NodeRef<Expression>(*first);
    }

    std::vector<NodeRef<Expression>> exprs{ *first };
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

    return m_nodes.Make<TupleCreator>(m_nodes.MakeSpan(exprs), true);
}

// The grammar follows jinja2/parser.py, loosest binding first:
// or, and, not, comparisons (chained, 'in', 'not in'), + -, ~, * / // %, **, unary + -,
// then postfix (attribute, subscript, slice, call), then filters and 'is' tests
ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseLogicalOr(LexScanner& lexer)
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
            left = NodeRef<Expression>(BinaryExpression::Make(m_nodes, BinaryExpression::LogicalOr, *left, *right));
        }
    }

    return left;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseLogicalAnd(LexScanner& lexer)
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
            left = NodeRef<Expression>(BinaryExpression::Make(m_nodes, BinaryExpression::LogicalAnd, *left, *right));
        }
    }

    return left;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseLogicalNot(LexScanner& lexer)
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

    return m_nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, *expr);
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseLogicalCompare(LexScanner& lexer)
{
    auto left = ParseMathPlusMinus(lexer);
    if (!left)
    {
        return left;
    }

    // Most chains are one comparison, which becomes a binary node: no heap list for it
    boost::container::small_vector<CompareExpression::Operand, 2> operands;
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
        operands.push_back(operand);
    }

    if (operands.empty())
    {
        return left;
    }

    // Every path returns `left`, so the result is never moved out
    if (operands.size() > 1)
    {
        left = m_nodes.Make<CompareExpression>(*left, m_nodes.MakeSpan(operands));
        return left;
    }

    // A single comparison keeps the plain binary node
    auto& operand = operands.front();
    NodeRef<Expression> result = BinaryExpression::Make(m_nodes, operand.operation, *left, operand.expr);
    if (operand.negated)
    {
        result = m_nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, result);
    }
    left = std::move(result);
    return left;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseMathPlusMinus(LexScanner& lexer)
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
        res = NodeRef<Expression>(BinaryExpression::Make(m_nodes, operation, *res, *right));
    }
    return res;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseStringConcat(LexScanner& lexer)
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
            left = NodeRef<Expression>(BinaryExpression::Make(m_nodes, BinaryExpression::StringConcat, *left, *right));
        }
    }
    return left;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseMathMulDiv(LexScanner& lexer)
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
        res = NodeRef<Expression>(BinaryExpression::Make(m_nodes, operation, *res, *right));
    }

    return res;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseMathPow(LexScanner& lexer)
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
            left = NodeRef<Expression>(BinaryExpression::Make(m_nodes, BinaryExpression::Pow, *left, *right));
        }
    }

    return left;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseUnaryPlusMinus(LexScanner& lexer, bool withFilter)
{
    const auto& tok = lexer.NextToken();
    ParseResult<NodeRef<Expression>> result;
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
        result = NodeRef<Expression>(
            m_nodes.Make<UnaryExpression>(tok == '+' ? UnaryExpression::UnaryPlus : UnaryExpression::UnaryMinus, *subExpr));
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

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseValueExpression(LexScanner& lexer)
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

        auto name = lexer.GetAsString(tok);
        if (name == "self")
        {
            return SelfRefExpression::Make(m_nodes);
        }
        auto ref = ValueRefExpression::Make(m_nodes, name);
        m_names.AddUse(ref);
        return ref;
    }
    case Token::IntegerNum:
    case Token::FloatNum:
        return MakeConstant(m_nodes, tok.value);
    case Token::String:
        return MakeConstant(m_nodes, ParseAdjacentStrings(lexer, tok.value));
    case Token::True:
        return MakeConstant(m_nodes, InternalValue(true));
    case Token::False:
        return MakeConstant(m_nodes, InternalValue(false));
    case Token::None:
        return MakeConstant(m_nodes, InternalValue(EmptyValue()));
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

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParsePostfix(LexScanner& lexer, NodeRef<Expression> valueRef)
{
    ParseResult<NodeRef<Expression>> result = valueRef;
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

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseFiltersAndTests(LexScanner& lexer,
                                                                                          NodeRef<Expression> valueRef)
{
    ParseResult<NodeRef<Expression>> result = valueRef;
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
            result = NodeRef<Expression>(m_nodes.Make<FilteredExpression>(m_nodes, *result, *filter));
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

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseTest(LexScanner& lexer, NodeRef<Expression> valueRef)
{
    const bool negated = lexer.EatIfEqual(Keyword::LogicalNot);

    // none, true and false are keywords to the lexer but test names to Jinja2
    Token nameTok = lexer.NextToken();
    std::string name;
    if (nameTok == Token::Identifier)
    {
        name = lexer.GetAsString(nameTok);
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

    NodeRef<Expression> result;
    try
    {
        result = IsExpression::Make(m_nodes, valueRef, name, params, FindRegisteredTester(name));
    }
    catch (const std::runtime_error&)
    {
        return MakeParseError(ErrorCode::UnexpectedException, nameTok);
    }

    if (negated)
    {
        result = m_nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, result);
    }

    return result;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseBracedExpressionOrTuple(LexScanner& lexer)
{
    NodeRef<Expression> result;

    bool isTuple = false;
    std::vector<NodeRef<Expression>> exprs;
    if (lexer.EatIfEqual(')'))
    {
        return m_nodes.Make<TupleCreator>(m_nodes.MakeSpan(exprs), true);
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
        result = m_nodes.Make<TupleCreator>(m_nodes.MakeSpan(exprs), true);
    }
    else
    {
        result = exprs[0];
    }

    return result;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseDictionary(LexScanner& lexer)
{
    NodeRef<Expression> result;

    std::vector<DictCreator::Item> items;
    if (lexer.EatIfEqual('}'))
    {
        return m_nodes.Make<DictCreator>(DictCreator::Items());
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

        items.push_back(DictCreator::Item{ *key, *expr });

    } while (lexer.EatIfEqual(',') && lexer.PeekNextToken() != '}');

    const auto& tok = lexer.NextToken();
    if (tok != '}')
    {
        return MakeParseError(ErrorCode::ExpectedCurlyBracket, tok);
    }

    result = m_nodes.Make<DictCreator>(m_nodes.MakeSpan(items));

    return result;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseTuple(LexScanner& lexer)
{
    NodeRef<Expression> result;

    std::vector<NodeRef<Expression>> exprs;
    if (lexer.EatIfEqual(']'))
    {
        return m_nodes.Make<TupleCreator>(m_nodes.MakeSpan(exprs));
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

    result = m_nodes.Make<TupleCreator>(m_nodes.MakeSpan(exprs));

    return result;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseCall(LexScanner& lexer, NodeRef<Expression> valueRef)
{
    NodeRef<Expression> result;

    ParseResult<CallParamsInfo> params = ParseCallParams(lexer);
    if (!params)
    {
        return MakeUnexpected(params.error());
    }

    result = m_nodes.Make<CallExpression>(m_nodes, valueRef, std::move(*params));

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
            paramName = lexer.GetAsString(tok);
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
    NodeRef<Expression> indexExpr;
    std::string attrName;
};

SubscriptParseResult<DotSubscript> ParseDotSubscript(NodeArena& nodes, LexScanner& lexer)
{
    DotSubscript result;
    // l.0 is l[0]; any other attribute is looked up by name
    const auto& tok = lexer.NextToken();
    if (tok == Token::Identifier)
    {
        result.attrName = lexer.GetAsString(tok);
    }
    else if (tok == Token::True || tok == Token::False || tok == Token::None)
    {
        result.attrName = lexer.GetAsString(tok);
    }
    else if ((tok == Token::IntegerNum || tok == Token::FloatNum) && GetIf<int64_t>(&tok.value))
    {
        result.indexExpr = MakeConstant(nodes, tok.value);
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
    NodeRef<Expression> parts[3];
    bool isSlice = false;
};

bool EndsSlicePart(LexScanner& lexer)
{
    const auto& next = lexer.PeekNextToken();
    return next == ']' || next == ':' || next == ',';
}

// One expression of the subscript, a sibling of the others
SubscriptParseResult<NodeRef<Expression>> ParseSubscriptPart(ExpressionParser& parser, LexScanner& lexer, SiblingOperators& siblings)
{
    siblings.Next();
    auto expr = parser.ParseFullExpression(lexer);
    if (!expr)
    {
        return MakeUnexpected(expr.error());
    }
    return NodeRef<Expression>(*expr);
}

// l[a, b] and l[] index with a tuple, as in Jinja2; first is the already parsed a, if any
SubscriptParseResult<NodeRef<Expression>> ParseSubscriptTuple(ExpressionParser& parser,
                                                              LexScanner& lexer,
                                                              SiblingOperators& siblings,
                                                              const NodeRef<Expression>& first)
{
    std::vector<NodeRef<Expression>> items;
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
    return NodeRef<Expression>(parser.Nodes().Make<TupleCreator>(parser.Nodes().MakeSpan(items), true));
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

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseSubscript(LexScanner& lexer, NodeRef<Expression> valueRef)
{
    NodeRef<Expression> indexExpr;
    std::string attrName;
    if (lexer.NextToken() == '.')
    {
        auto dot = ParseDotSubscript(m_nodes, lexer);
        if (!dot)
        {
            return MakeUnexpected(dot.error());
        }
        indexExpr = dot->indexExpr;
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
            return m_nodes.Make<SliceExpression>(valueRef, parts[0], parts[1], parts[2]);
        }
        indexExpr = parts[0];
    }

    // Consecutive subscripts share one node: a.b[0].c
    auto subscript = m_nodes.As<SubscriptExpression>(valueRef);
    if (!subscript)
    {
        subscript = MakeSubscript(valueRef, attrName);
    }
    m_nodes[subscript].AddIndex(m_nodes, indexExpr, std::move(attrName));

    return subscript;
}

// A subscript of `value` whose first index is the attribute `attrName` (empty for an item):
// `loop.<attribute>` reads the attribute of a for loop's object directly (0117 P1)
NodeRef<SubscriptExpression> ExpressionParser::MakeSubscript(NodeRef<Expression> value, const std::string& attrName)
{
    if (!attrName.empty() && m_nodes[value].GetKind() == NodeKind::NameRef && m_nodes.Get<ValueRefExpression>(value).GetName(m_nodes) == "loop")
    {
        if (const auto attr = FindLoopAttr(attrName); attr != LoopAttr::None)
        {
            return m_nodes.Make<LoopAttrExpression>(value, attr);
        }
    }
    return m_nodes.Make<SubscriptExpression>(value);
}

ExpressionParser::ParseResult<NodeRef<ExpressionFilter>> ExpressionParser::ParseFilterExpression(LexScanner& lexer)
{
    NodeRef<ExpressionFilter> result;

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

            std::string name = lexer.GetAsString(tok);
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

            auto filter = ExpressionFilter::Make(m_nodes, name, *params, FindRegisteredFilter(name));
            if (result && !AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
            }
            if (result)
            {
                m_nodes[filter].SetParentFilter(result);
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

ExpressionParser::ParseResult<NodeRef<IfExpression>> ExpressionParser::ParseIfExpression(LexScanner& lexer)
{
    NodeRef<IfExpression> result;

    try
    {
        auto testExpr = ParseLogicalOr(lexer);
        if (!testExpr)
        {
            return MakeUnexpected(testExpr.error());
        }

        ParseResult<NodeRef<Expression>> altValue;
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

        result = m_nodes.Make<IfExpression>(*testExpr, *altValue);
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
