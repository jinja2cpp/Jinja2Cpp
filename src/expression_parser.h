#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "lexer.h"
#include "expression_evaluator.h"
#include "renderer.h"

namespace jinja2
{
class ExpressionParser
{
public:
    ExpressionParser();
    RendererPtr Parse(LexScanner& lexer);
    ExpressionEvaluatorPtr<FullExpressionEvaluator> ParseFullExpression(LexScanner& lexer, bool includeIfPart = true);
private:
    ExpressionEvaluatorPtr<Expression> ParseLogicalNot(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseLogicalOr(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseLogicalAnd(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseLogicalCompare(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseStringConcat(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseMathPow(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseMathMulDiv(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseMathPlusMinus(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseUnaryPlusMinus(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseValueExpression(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseBracedExpressionOrTuple(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseDictionary(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseTuple(LexScanner& lexer);
    ExpressionEvaluatorPtr<Expression> ParseCall(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef);
    CallParams ParseCallParams(LexScanner& lexer, bool& isValid);
    ExpressionEvaluatorPtr<Expression> ParseSubscript(LexScanner& lexer, ExpressionEvaluatorPtr<Expression> valueRef);
    ExpressionEvaluatorPtr<ExpressionFilter> ParseFilterExpression(LexScanner& lexer);
    ExpressionEvaluatorPtr<IfExpression> ParseIfExpression(LexScanner& lexer);

private:
    ComposedRenderer* m_topLevelRenderer;
};

} // jinja2

#endif // EXPRESSION_PARSER_H
