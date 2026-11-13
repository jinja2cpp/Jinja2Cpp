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
#include <memory>
#include <string>
#include <vector>

namespace jinja2
{
inline namespace v2
{
// A subscript by a tuple of slices (TupleSliceExpression) compiles in Jinja2 only where its
// optimizer folds an expression around it into a constant of a safe repr (Optimizer, `{% set %}`,
// a test, a loop's filter), or the compiler folds the output it is in into a str (visit_Output):
// unfolded, it is invalid Python (docs/tasks/0445, 0471). Whether a fold succeeds may take
// evaluating it, which only the sealed tree can do, so the parser records the expressions
// around such subscripts that may fold and the Load evaluates them (FoldCheckPasses)
struct FoldCandidate
{
    NodeRef<Expression> node;
    // The tuple-slice subscripts inside it, numbered in the order the parser made them: [from, to)
    unsigned from = 0;
    unsigned to = 0;
    // Its PrepareFold, never 0
    std::uint8_t fold = 0;
    // The whole value of an output: folds whatever its value, which is printed
    bool output = false;
    // With StrictUndefined, whether its failing evaluation fails the Load: Jinja2's as_const of
    // an arithmetic operator, a comparison, `not`, a subscript, a filter or a test turns the
    // failure into Impossible, so it only does not fold; that of `and`, `or`, `~`, a conditional
    // and an output lets the UndefinedError out (docs/tasks/0484)
    bool failsLoad = true;
};

// The subscripts of one top-level expression and where it may fold; the error is the Load's
// if one of them stays unfolded
struct FoldCheck
{
    std::vector<FoldCandidate> candidates;
    unsigned from = 0;
    unsigned to = 0;
    bool strictUndefined = false;
    ParseError error;
};

class RenderContext;
// Whether every subscript of the check is folded away, evaluated in `context`, a context on the
// sealed tree. With StrictUndefined, Jinja2's fold also fails the Load where its optimizer
// stringifies or tests the undefined such a subscript gives, so any candidate whose evaluation
// fails fails the check
bool FoldCheckPasses(const FoldCheck& check, RenderContext& context);

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

    // The folds the Load checks on the sealed tree (FoldCheck): the parse-wide list of what is
    // left to check after the parse, as the names are
    // Only a template with a subscript by a tuple of slices has them: the rest hold one null pointer
    // pointer, out of line, so that the parsers that inline this class's members do not grow
    void AddFoldCheck(FoldCheck check);
    // Null when there are none
    [[nodiscard]] const std::vector<FoldCheck>* GetFoldChecks() const { return m_fold ? FoldChecks() : nullptr; }
    // Those of the top-level expression being parsed (ExpressionParser::AddFoldCandidate)
    std::vector<FoldCandidate>& FoldCandidates();
    void DropFoldCandidates();

private:
    std::vector<Name> m_names;
    struct FoldState;
    struct FoldStateDeleter
    {
        void operator()(FoldState* state) const;
    };
    FoldState& Fold();
    [[nodiscard]] const std::vector<FoldCheck>* FoldChecks() const;
    std::unique_ptr<FoldState, FoldStateDeleter> m_fold;
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
    // The expressions that follow are output in volatile code (Parse sets it for `{{ }}`; a
    // `print` statement sets it itself): their filters and tests are not folded
    void SetVolatile(bool isVolatile) { m_volatile = isVolatile; }
    // Each top-level expression that follows is a value of an output (a `print` statement's):
    // Jinja2 folds it whole whatever its value (FoldCandidate::output)
    void SetOutputExpressions(bool isOutput) { m_outputExpressions = isOutput; }
    ParseResult<NodeRef<Expression>> ParseFullExpression(LexScanner& lexer, bool includeIfPart = true);
    // Jinja2's parse_tuple without parentheses: 'a, b' is a tuple, 'a' stays an expression
    ParseResult<NodeRef<Expression>> ParseTupleOrExpression(LexScanner& lexer, bool includeIfPart = true);
    // The arguments of a call, a filter or a test, `*args` and `**kwargs` included
    ParseResult<CallArgsInfo> ParseCallArgs(LexScanner& lexer);
    ParseResult<NodeRef<ExpressionFilter>> ParseFilterExpression(LexScanner& lexer);
    // The names a filter `name` called with `params` reads besides its own: `applymacro` reads a
    // macro by name, `map`, `select` and the like a filter or test that may fall back to a value
    // (NameResolver::AddRead)
    void NoteNamedCallables(const std::string& name, const CallArgsInfo& params);
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
    // foldFrom: m_tupleSliceSubscripts before the operand (NoteFoldCandidate)
    ParseResult<NodeRef<Expression>> ParseOperandSuffix(LexScanner& lexer, NodeRef<Expression> operand, bool withFilter, unsigned foldFrom);
    // A name, literal or bracketed expression with its postfix operators, and with withFilter its filters and tests
    ParseResult<NodeRef<Expression>> ParseValueExpression(LexScanner& lexer, bool withFilter);
    ParseResult<NodeRef<Expression>> ParsePostfix(LexScanner& lexer, NodeRef<Expression> valueRef);
    ParseResult<NodeRef<Expression>> ParseFiltersAndTests(LexScanner& lexer, NodeRef<Expression> valueRef, unsigned foldFrom);
    ParseResult<NodeRef<Expression>> ParseTest(LexScanner& lexer, NodeRef<Expression> valueRef);
    ParseResult<NodeRef<Expression>> ParseBracedExpressionOrTuple(LexScanner& lexer);
    ParseResult<NodeRef<Expression>> ParseDictionary(LexScanner& lexer);
    ParseResult<NodeRef<Expression>> ParseTuple(LexScanner& lexer);
    ParseResult<NodeRef<Expression>> ParseCall(LexScanner& lexer, NodeRef<Expression> valueRef);
    ParseResult<NodeRef<Expression>> ParseSubscript(LexScanner& lexer, NodeRef<Expression> valueRef);
    NodeRef<SubscriptExpression> MakeSubscript(NodeRef<Expression> value, const std::string& attrName);
    // valueOffset: where the value before `if` starts, the position Jinja2 reports for the conditional
    ParseResult<NodeRef<IfExpression>> ParseIfExpression(LexScanner& lexer, size_t valueOffset);
    // `node`, parsed since m_tupleSliceSubscripts was `from`, is where Jinja2 may fold the
    // tuple-slice subscripts made since: recorded if any were and it may fold at all
    void NoteFoldCandidate(NodeRef<Expression> node, unsigned from, bool output = false)
    {
        if (m_tupleSliceSubscripts != from)
        {
            AddFoldCandidate(node, from, output);
        }
    }

    // In a top-level expression (FoldCheck): Parse's output, or a statement's ParseFullExpression,
    // which do not nest: the rest are nested in one (m_depth counts them)
    [[nodiscard]] bool InFoldRoot() const { return m_outputRoot || m_depth != 0; }
    void AddFoldCandidate(NodeRef<Expression> node, unsigned from, bool output);
    // At the end of the top-level expression starting at `tok`: an error if one of its subscripts
    // cannot fold, else the check goes to the Load
    nonstd::expected<void, ParseError> FinishFoldRoot(const Token& tok, unsigned rootFrom);
    // At the end of ParseFullExpression, when its value made tuple-slice subscripts: the value
    // is a candidate, and a top-level expression's check is made (out of line: few do)
    nonstd::expected<void, ParseError> NoteFullExpressionFold(NodeRef<Expression> result, unsigned from, const Token& tok);
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
    // SetOutputExpressions
    bool m_outputExpressions = false;
    // Parse's output is the top-level expression (InFoldRoot). The expressions in a top-level
    // one that may fold its tuple-slice subscripts away go to UnknownCallables::FoldCandidates,
    // which the parsers of one template share, so that a parser, made for each tag, owns no list
    bool m_outputRoot = false;
    // In the else branch of a conditional, parsed as if it were not nested (ParseIfExpression)
    bool m_inElseChain = false;
    // Subscripts by a tuple holding a slice (`[1, 2][1:2, 3]`) made so far: Jinja2 compiles one
    // only where it folds an expression around it, else it writes invalid Python (FoldCheck)
    unsigned m_tupleSliceSubscripts = 0;
    // The undefined the last of them folds to
    NodeRef<Expression> m_tupleSliceValue;
    // AutoescapeMode::Prompt: the template's string literals are prompt-safe, Markup (docs/tasks/0358)
    bool m_trustedLiterals = false;
    // Settings::undefinedPolicy is Strict: printing the folded undefined fails, so Jinja2 does not fold it
    bool m_strictUndefined = false;
    // Nesting level of the expression being parsed, bounded by Settings::Limits::maxExpressionDepth
    unsigned m_depth = 0;
    // Operators chained on the current path (a + b + c, x|f|g, a.b.c, - - x), bounded by
    // Settings::Limits::maxExpressionOperators
    unsigned m_operators = 0;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_EXPRESSION_PARSER_H
