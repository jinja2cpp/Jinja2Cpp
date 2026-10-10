#ifndef JINJA2CPP_SRC_EXPRESSION_PARSER_H
#define JINJA2CPP_SRC_EXPRESSION_PARSER_H

#include "error_handling.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "lexer.h"
#include "name_resolver.h"
#include "node_arena.h"
#include "renderer.h"

#include <jinja2cpp/template_env.h>

#include <nonstd/expected.hpp>

#include <cstdint>
#include <string>

namespace jinja2
{
class ExpressionParser
{
public:
    template<typename T>
    using ParseResult = nonstd::expected<T, ParseError>;

    // The nodes go to `nodes`, which outlives the parser
    ExpressionParser(const Settings& settings, TemplateEnv* env, NodeArena& nodes, NameResolver& names);
    ParseResult<NodeRef<IRendererBase>> Parse(LexScanner& lexer);
    // Before each of several top-level expressions of one statement (with bindings, macro
    // defaults): their operators do not add up
    void NextTopLevelExpression() { m_operators = 0; }
    ParseResult<NodeRef<Expression>> ParseFullExpression(LexScanner& lexer, bool includeIfPart = true);
    // Jinja2's parse_tuple without parentheses: 'a, b' is a tuple, 'a' stays an expression
    ParseResult<NodeRef<Expression>> ParseTupleOrExpression(LexScanner& lexer, bool includeIfPart = true);
    ParseResult<CallParamsInfo> ParseCallParams(LexScanner& lexer);
    ParseResult<NodeRef<ExpressionFilter>> ParseFilterExpression(LexScanner& lexer);
    // Settings::finalize as a callable; undefined if it is not set
    [[nodiscard]] const InternalValue& GetFinalize() const { return m_finalize; }
    [[nodiscard]] NodeArena& Nodes() const { return m_nodes; }

    // Binding strength of the binary operators, loosest first, as in jinja2/parser.py: 'not'
    // binds between 'and' and the comparisons, unary + - tighter than '**'
    enum class Precedence : std::uint8_t
    {
        None, // not a binary operator
        Or,
        And,
        Not,
        Compare,
        PlusMinus,
        Concat,
        MulDiv,
        Pow,
    };

private:
    // An expression of binary operators no looser than minPrecedence, by precedence climbing
    ParseResult<NodeRef<Expression>> ParseBinary(LexScanner& lexer, Precedence minPrecedence);
    // 'not' and its operand; the next token is the 'not'
    ParseResult<NodeRef<Expression>> ParseLogicalNot(LexScanner& lexer);
    // A chain of comparisons after its left operand; the next token is the first operator
    ParseResult<NodeRef<Expression>> ParseComparisons(LexScanner& lexer,
                                                      NodeRef<Expression> left,
                                                      BinaryExpression::Operation operation,
                                                      bool negated);
    ParseResult<NodeRef<Expression>> ParseUnaryPlusMinus(LexScanner& lexer, bool withFilter = true);
    // The postfix operators, filters and tests after an operand
    ParseResult<NodeRef<Expression>> ParseOperandSuffix(LexScanner& lexer, NodeRef<Expression> operand, bool withFilter);
    ParseResult<NodeRef<Expression>> ParseValueExpression(LexScanner& lexer);
    ParseResult<NodeRef<Expression>> ParsePostfix(LexScanner& lexer, NodeRef<Expression> valueRef);
    ParseResult<NodeRef<Expression>> ParseFiltersAndTests(LexScanner& lexer, NodeRef<Expression> valueRef);
    ParseResult<NodeRef<Expression>> ParseTest(LexScanner& lexer, NodeRef<Expression> valueRef);
    ParseResult<NodeRef<Expression>> ParseBracedExpressionOrTuple(LexScanner& lexer);
    ParseResult<NodeRef<Expression>> ParseDictionary(LexScanner& lexer);
    ParseResult<NodeRef<Expression>> ParseTuple(LexScanner& lexer);
    ParseResult<NodeRef<Expression>> ParseCall(LexScanner& lexer, NodeRef<Expression> valueRef);
    ParseResult<NodeRef<Expression>> ParseSubscript(LexScanner& lexer, NodeRef<Expression> valueRef);
    NodeRef<SubscriptExpression> MakeSubscript(NodeRef<Expression> value, const std::string& attrName);
    ParseResult<NodeRef<IfExpression>> ParseIfExpression(LexScanner& lexer);
    // Counts one more chained operator; false past MaxExpressionOperators
    bool AddOperator();
    // The filter or test the environment adds under this name, as a callable; undefined if there is none
    [[nodiscard]] InternalValue FindRegisteredFilter(const std::string& name) const;
    [[nodiscard]] InternalValue FindRegisteredTester(const std::string& name) const;

    TemplateEnv* m_env = nullptr;
    NodeArena& m_nodes;
    // Told of each name expression, for slots (0117 P1)
    NameResolver& m_names;
    // Settings::finalize as a callable; undefined if it is not set
    InternalValue m_finalize;
    // Nesting level of the expression being parsed, bounded by MaxExpressionDepth
    unsigned m_depth = 0;
    // Operators chained on the current path (a + b + c, x|f|g, a.b.c, - - x), bounded by MaxExpressionOperators
    unsigned m_operators = 0;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_EXPRESSION_PARSER_H
