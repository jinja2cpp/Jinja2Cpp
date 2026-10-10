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

ParseError::ParseError(const ParseError&) = default;
ParseError::ParseError(ParseError&& other) noexcept
    : errorCode(other.errorCode)
    , errorToken(std::move(other.errorToken))
    , relatedTokens(std::move(other.relatedTokens))
{
}
ParseError& ParseError::operator=(const ParseError&) = default;
ParseError& ParseError::operator=(ParseError&& error) noexcept
{
    if (this == &error)
    {
        return *this;
    }

    std::swap(errorCode, error.errorCode);
    std::swap(errorToken, error.errorToken);
    std::swap(relatedTokens, error.relatedTokens);

    return *this;
}
ParseError::~ParseError() = default;

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

    auto value = ParseBinary(lexer, Precedence::Or);
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
// then postfix (attribute, subscript, slice, call), then filters and 'is' tests.
// The binary levels are parsed by precedence climbing (0152): an operand with no operator
// after it costs one ParseBinary call, not one call per level
namespace
{
struct BinaryOperator
{
    ExpressionParser::Precedence precedence = ExpressionParser::Precedence::None;
    BinaryExpression::Operation operation{};
    // 'not in': two tokens
    bool negated = false;
};

// The binary operator the next token starts; Precedence::None if it starts none
BinaryOperator PeekBinaryOperator(LexScanner& lexer)
{
    using Precedence = ExpressionParser::Precedence;
    const auto& tok = lexer.PeekNextToken();
    switch (tok.type)
    {
    case Token::Equal:
        return { Precedence::Compare, BinaryExpression::LogicalEq };
    case Token::NotEqual:
        return { Precedence::Compare, BinaryExpression::LogicalNe };
    case '<':
        return { Precedence::Compare, BinaryExpression::LogicalLt };
    case '>':
        return { Precedence::Compare, BinaryExpression::LogicalGt };
    case Token::GreaterEqual:
        return { Precedence::Compare, BinaryExpression::LogicalGe };
    case Token::LessEqual:
        return { Precedence::Compare, BinaryExpression::LogicalLe };
    case '+':
        return { Precedence::PlusMinus, BinaryExpression::Plus };
    case '-':
        return { Precedence::PlusMinus, BinaryExpression::Minus };
    case '*':
        return { Precedence::MulDiv, BinaryExpression::Mul };
    case '/':
        return { Precedence::MulDiv, BinaryExpression::Div };
    case Token::DivDiv:
        return { Precedence::MulDiv, BinaryExpression::DivInteger };
    case '%':
        return { Precedence::MulDiv, BinaryExpression::DivRemainder };
    case Token::MulMul:
        return { Precedence::Pow, BinaryExpression::Pow };
    case Token::Identifier:
        switch (tok.keyword)
        {
        case Keyword::LogicalOr:
            return { Precedence::Or, BinaryExpression::LogicalOr };
        case Keyword::LogicalAnd:
            return { Precedence::And, BinaryExpression::LogicalAnd };
        case Keyword::In:
            return { Precedence::Compare, BinaryExpression::In };
        case Keyword::LogicalNot:
        {
            // 'not' after an operand is only an operator in 'not in'
            lexer.EatToken();
            const bool notIn = lexer.PeekNextToken().keyword == Keyword::In;
            lexer.ReturnToken();
            if (notIn)
            {
                return { Precedence::Compare, BinaryExpression::In, true };
            }
            return {};
        }
        default:
            return {};
        }
    default:
        // '~' binds tighter than '+' and looser than '*', as in Jinja2; it has no Token::Type enumerator
        if (tok == '~')
        {
            return { Precedence::Concat, BinaryExpression::StringConcat };
        }
        return {};
    }
}

ExpressionParser::Precedence Tighter(ExpressionParser::Precedence precedence)
{
    return static_cast<ExpressionParser::Precedence>(static_cast<std::uint8_t>(precedence) + 1);
}
} // namespace

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseBinary(LexScanner& lexer, Precedence minPrecedence)
{
    // The operators of one call combine like the old one-function-per-level chains: each
    // right operand starts counting from where this call started (OperatorChain)
    OperatorChain chain(m_operators);
    auto left = minPrecedence <= Precedence::Not && lexer.PeekNextToken().keyword == Keyword::LogicalNot ? ParseLogicalNot(lexer)
                                                                                                    : ParseUnaryPlusMinus(lexer);
    while (left)
    {
        const auto op = PeekBinaryOperator(lexer);
        if (op.precedence < minPrecedence)
        {
            break;
        }
        if (op.precedence == Precedence::Compare)
        {
            // Every path assigns `left`, the one value returned, so the result is never moved out
            left = ParseComparisons(lexer, *left, op.operation, op.negated);
            continue;
        }

        const auto& tok = lexer.NextToken();
        chain.BeforeRight();
        // Every binary operator is left-associative, '**' included: Jinja2 reads 2 ** 3 ** 2 as 64
        auto right = op.precedence == Precedence::Pow ? ParseUnaryPlusMinus(lexer) : ParseBinary(lexer, Tighter(op.precedence));
        if (!right)
        {
            left = std::move(right);
        }
        else if (!chain.AfterRight(MaxExpressionOperators))
        {
            left = MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
        }
        else
        {
            left = NodeRef<Expression>(BinaryExpression::Make(m_nodes, op.operation, *left, *right));
        }
    }

    return left;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseLogicalNot(LexScanner& lexer)
{
    // 'not a == b' is 'not (a == b)'
    lexer.EatToken();

    if (!AddOperator())
    {
        return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
    }
    auto expr = ParseBinary(lexer, Precedence::Not);
    if (!expr)
    {
        return expr;
    }

    return m_nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, *expr);
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseComparisons(LexScanner& lexer,
                                                                                      NodeRef<Expression> left,
                                                                                      BinaryExpression::Operation operation,
                                                                                      bool negated)
{
    // Most chains are one comparison, which becomes a binary node: no heap list for it
    boost::container::small_vector<CompareExpression::Operand, 2> operands;
    for (;;)
    {
        lexer.EatToken();
        if (negated)
        {
            lexer.EatToken();
        }
        // Operands of a comparison do not count as one chain: a == b == c does not nest
        auto right = ParseBinary(lexer, Precedence::PlusMinus);
        if (!right)
        {
            return right;
        }
        CompareExpression::Operand operand;
        operand.operation = operation;
        operand.negated = negated;
        operand.expr = *right;
        operands.push_back(operand);

        const auto next = PeekBinaryOperator(lexer);
        if (next.precedence != Precedence::Compare)
        {
            break;
        }
        operation = next.operation;
        negated = next.negated;
    }

    if (operands.size() > 1)
    {
        return m_nodes.Make<CompareExpression>(left, m_nodes.MakeSpan(operands));
    }

    // A single comparison keeps the plain binary node
    auto& operand = operands.front();
    NodeRef<Expression> result = BinaryExpression::Make(m_nodes, operand.operation, left, operand.expr);
    if (operand.negated)
    {
        result = m_nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, result);
    }
    return result;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseUnaryPlusMinus(LexScanner& lexer, bool withFilter)
{
    const auto& tok = lexer.PeekNextToken();
    if (tok == '+' || tok == '-')
    {
        lexer.EatToken();
        if (!AddOperator())
        {
            return MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
        }
        // Filters after a unary operand apply to the negated value: -x|abs is (-x)|abs
        auto subExpr = ParseUnaryPlusMinus(lexer, false);
        if (!subExpr)
        {
            return subExpr;
        }
        auto unary = m_nodes.Make<UnaryExpression>(tok == '+' ? UnaryExpression::UnaryPlus : UnaryExpression::UnaryMinus, *subExpr);
        return ParseOperandSuffix(lexer, unary, withFilter);
    }

    auto value = ParseValueExpression(lexer);
    if (!value)
    {
        return value;
    }
    return ParseOperandSuffix(lexer, *value, withFilter);
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseOperandSuffix(LexScanner& lexer,
                                                                                        NodeRef<Expression> operand,
                                                                                        bool withFilter)
{
    // Most operands have neither a postfix nor a filter: no parser runs for them, and no
    // result is assigned twice
    const auto& next = lexer.PeekNextToken();
    if (next == '.' || next == '[' || next == '(')
    {
        auto result = ParsePostfix(lexer, operand);
        if (!result)
        {
            return result;
        }
        operand = *result;
    }
    const auto& filterStart = lexer.PeekNextToken();
    if (withFilter && (filterStart == '|' || filterStart == '(' || filterStart.keyword == Keyword::Is))
    {
        return ParseFiltersAndTests(lexer, operand);
    }

    return operand;
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
        // Adjacent literals are rare: copy the token's value once for the usual single one
        if (lexer.PeekNextToken() != Token::String)
        {
            return MakeConstant(m_nodes, tok.value);
        }
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
        auto testExpr = ParseBinary(lexer, Precedence::Or);
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
