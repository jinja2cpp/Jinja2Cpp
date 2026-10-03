#ifndef JINJA2CPP_SRC_EXPRESSION_PARSER_H
#define JINJA2CPP_SRC_EXPRESSION_PARSER_H

#include "lexer.h"
#include "error_handling.h"
#include "expression_evaluator.h"
#include "renderer.h"

#include <nonstd/expected.hpp>
#include <jinja2cpp/template_env.h>

namespace jinja2
{
class ExpressionParser
{
public:
    template<typename T>
    using ParseResult = nonstd::expected<T, ParseError>;

    explicit ExpressionParser(const Settings& settings, TemplateEnv* env = nullptr);
    ParseResult<RendererPtr> Parse(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<FullExpressionEvaluator>> ParseFullExpression(LexScanner& lexer, bool includeIfPart = true);
    // Jinja2's parse_tuple without parentheses: 'a, b' is a tuple, 'a' stays an expression
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseTupleOrExpression(LexScanner& lexer, bool includeIfPart = true);
    ParseResult<CallParamsInfo> ParseCallParams(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<ExpressionFilter>> ParseFilterExpression(LexScanner& lexer);
    // Settings::finalize as a callable; undefined if it is not set
    [[nodiscard]] const InternalValue& GetFinalize() const { return m_finalize; }
private:
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseLogicalOr(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseLogicalAnd(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseLogicalNot(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseLogicalCompare(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseMathPlusMinus(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseStringConcat(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseMathMulDiv(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseMathPow(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseUnaryPlusMinus(LexScanner& lexer, bool withFilter = true);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseValueExpression(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParsePostfix(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseFiltersAndTests(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseTest(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseBracedExpressionOrTuple(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseDictionary(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseTuple(LexScanner& lexer);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseCall(LexScanner& lexer, const ExpressionEvaluatorPtr<Expression>& valueRef);
    ParseResult<ExpressionEvaluatorPtr<Expression>> ParseSubscript(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef);
    ParseResult<ExpressionEvaluatorPtr<IfExpression>> ParseIfExpression(LexScanner& lexer);
    // Counts one more chained operator; false past MaxExpressionOperators
    bool AddOperator();
    // The filter or test the environment adds under this name, as a callable; undefined if there is none
    [[nodiscard]] InternalValue FindRegisteredFilter(const std::string& name) const;
    [[nodiscard]] InternalValue FindRegisteredTester(const std::string& name) const;

    TemplateEnv* m_env = nullptr;
    // Settings::finalize as a callable; undefined if it is not set
    InternalValue m_finalize;
    // Nesting level of the expression being parsed, bounded by MaxExpressionDepth
    unsigned m_depth = 0;
    // Operators in left-associative chains (a + b + c, x|f|g, a.b.c), bounded by MaxExpressionOperators
    unsigned m_operators = 0;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_EXPRESSION_PARSER_H
