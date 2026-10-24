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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace jinja2
{
inline namespace v2
{
// Jinja2 rejects a filter or test it does not know when it compiles the template, except in
// an `if` statement or an inline if, which leave the check to the call (Frame.soft_frame in
// jinja2/compiler.py). The expression parsers of one template report each unknown name here.
// Unknown names are rare, so where the tag is is only worked out for a tag that adds one: the
// template parser then places the names it added, and once the template has parsed without a
// syntax error reports the first name left
class UnknownCallables
{
public:
    // Whether Jinja2 checks a name when it compiles it
    enum class Check : std::uint8_t
    {
        AsTag,    // as the tag's place in the template says
        Now,      // in a part of a statement with a frame of its own (a loop's filter, a macro's defaults)
        Deferred, // in the condition of an `if`
    };

    struct Name
    {
        Token token;
        bool isTest = false;
        Check check = Check::AsTag;
        // Where Jinja2 compiles the name: 0 in the template's body, else 1 + the offset of the
        // innermost block. Blocks compile after the body, in the order they start
        std::size_t unit = 0;
    };

    // Settings::userCallablesAsFilters: names are looked up when they run, none is reported
    explicit UnknownCallables(bool lookUpWhenRun)
        : m_lookUpWhenRun(lookUpWhenRun)
    {
    }

    [[nodiscard]] Check GetCheck() const { return m_check; }
    void SetCheck(Check check) { m_check = check; }

    void Add(const Token& token, bool isTest)
    {
        if (m_check != Check::Deferred && !m_lookUpWhenRun)
        {
            m_names.push_back(Name{ token, isTest, m_check, 0 });
            m_count = m_names.size();
        }
    }
    [[nodiscard]] std::size_t Mark() const { return m_count; }
    // The names added since `mark` are not checked: they are in an inline if, or in an
    // expression the parser gave up
    void DeferSince(std::size_t mark)
    {
        m_names.resize(mark);
        m_count = mark;
    }
    // The tag that added the names since `mark` is where Jinja2 defers the check or not, in `unit`
    void Place(std::size_t mark, bool deferred, std::size_t unit)
    {
        auto out = m_names.begin() + static_cast<std::ptrdiff_t>(mark);
        for (auto in = out; in != m_names.end(); ++in)
        {
            if (in->check == Check::AsTag && deferred)
            {
                continue;
            }
            in->unit = unit;
            *out++ = *in;
        }
        m_names.erase(out, m_names.end());
        m_count = m_names.size();
    }

    // The name Jinja2 reports, null if none
    [[nodiscard]] const Name* First() const
    {
        const Name* first = nullptr;
        for (const auto& name : m_names)
        {
            if (!first || name.unit < first->unit)
            {
                first = &name;
            }
        }
        return first;
    }

private:
    std::vector<Name> m_names;
    // m_names.size(), which every expression reads twice: a load, not a division by sizeof(Name)
    // (docs/tasks/0205)
    std::size_t m_count = 0;
    Check m_check = Check::AsTag;
    bool m_lookUpWhenRun = false;
};

// Says how Jinja2 checks the names of one part of a statement
class CheckGuard
{
public:
    CheckGuard(UnknownCallables& unknown, UnknownCallables::Check check)
        : m_unknown(unknown)
        , m_saved(unknown.GetCheck())
    {
        m_unknown.SetCheck(check);
    }
    ~CheckGuard() { m_unknown.SetCheck(m_saved); }
    CheckGuard(const CheckGuard&) = delete;
    CheckGuard& operator=(const CheckGuard&) = delete;

private:
    UnknownCallables& m_unknown;
    UnknownCallables::Check m_saved;
};

class ExpressionParser
{
public:
    template<typename T>
    using ParseResult = nonstd::expected<T, ParseError>;

    // The nodes go to `nodes`, which outlives the parser; unknown filters and tests to `unknown`
    ExpressionParser(const Settings& settings, TemplateEnv* env, NodeArena& nodes, NameResolver& names, UnknownCallables& unknown);
    // `{{ ... }}`; isVolatile: in volatile code (RenderContext::IsVolatileAutoescape), where
    // Jinja2 folds no filter or test and escapes a folded value by the static setting
    ParseResult<NodeRef<IRendererBase>> Parse(LexScanner& lexer, bool isVolatile = false);
    // Before each of several top-level expressions of one statement (with bindings, macro
    // defaults): their operators do not add up
    void NextTopLevelExpression() { m_operators = 0; }
    ParseResult<NodeRef<Expression>> ParseFullExpression(LexScanner& lexer, bool includeIfPart = true);
    // Jinja2's parse_tuple without parentheses: 'a, b' is a tuple, 'a' stays an expression
    ParseResult<NodeRef<Expression>> ParseTupleOrExpression(LexScanner& lexer, bool includeIfPart = true);
    // allowUnpacking: `*args` and `**kwargs` arguments, which only calls take so far: filters and
    // tests bind their arguments at Load (docs/tasks/0304)
    ParseResult<CallParamsInfo> ParseCallParams(LexScanner& lexer, bool allowUnpacking = false);
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
    // A name, literal or bracketed expression with its postfix operators, and with withFilter its filters and tests
    ParseResult<NodeRef<Expression>> ParseValueExpression(LexScanner& lexer, bool withFilter);
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
    // Counts one more chained operator; false past Settings::Limits::maxExpressionOperators
    bool AddOperator();
    // The filter or test the environment adds under this name, as a callable; undefined if there is none
    [[nodiscard]] InternalValue FindRegisteredFilter(const std::string& name) const;
    [[nodiscard]] InternalValue FindRegisteredTester(const std::string& name) const;

    TemplateEnv* m_env = nullptr;
    NodeArena& m_nodes;
    // Told of each name expression, for slots (0117 P1)
    NameResolver& m_names;
    UnknownCallables& m_unknown;
    // Settings::finalize as a callable; undefined if it is not set
    InternalValue m_finalize;
    // Parse's isVolatile
    bool m_volatile = false;
    // Nesting level of the expression being parsed, bounded by Settings::Limits::maxExpressionDepth
    unsigned m_depth = 0;
    // Operators chained on the current path (a + b + c, x|f|g, a.b.c, - - x), bounded by
    // Settings::Limits::maxExpressionOperators
    unsigned m_operators = 0;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_EXPRESSION_PARSER_H
