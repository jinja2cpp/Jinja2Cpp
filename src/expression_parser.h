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
    // Its PrepareFold; 0 only for a candidate with leadingOperands, which never folds
    std::uint8_t fold = 0;
    // The whole value of an output: folds whatever its value, which is printed
    bool output = false;
    // With StrictUndefined, whether its failing evaluation fails the Load: Jinja2's as_const of
    // an arithmetic operator, a comparison, `not`, a subscript, a filter or a test turns the
    // failure into Impossible, so it only does not fold (docs/tasks/0484). An `and`, `or`, `~` or
    // conditional lets out only that of its own bool() or str(), which FoldEvaluator decides
    // (docs/tasks/0527, 0546). Never an output's: visit_Output catches what its fold raises and
    // leaves the value to the optimizer, as a candidate of its own (docs/tasks/0556)
    bool failsLoad = true;
    // With StrictUndefined, not an output's: the operands before the first that does not fold of an `and`, `or`
    // (its left one) or `~`, or a conditional's test. Jinja2's as_const evaluates them in turn, and the strict
    // undefined one of them gives fails the compile in bool() or str() before an operand that does not fold is
    // reached: `[1][:, :] or x`, `[1][:, :] ~ x`, `x if [1][:, :]` (docs/tasks/0536)
    std::uint32_t leadingOperands = 0;
};

// What the parser counts with the tuple-slice subscripts (ExpressionParser::AddFoldMark): the
// other constants whose folds decide the compile, numbered with the subscripts so that the
// expressions around them are candidates too (docs/tasks/0555, 0564)
enum class FoldMarkKind : std::uint8_t
{
    // An integer literal past MaxIntStrDigits digits: the code generator's repr() of it fails the
    // compile unless a fold around it takes it away, as for a tuple-slice subscript
    StrDigits,
    // Needs no fold: a constant whose fold may give an integer past MaxIntStrDigits digits (`*`,
    // `**`, `|int`, a literal near the limit), or, with StrictUndefined, a strict undefined (a
    // filter or subscript of a constant). The Load evaluates the candidates around it
    Watch,
};

struct FoldMark
{
    // Its number among the tuple-slice subscripts
    unsigned number = 0;
    FoldMarkKind kind = FoldMarkKind::Watch;
};

// The subscripts of one top-level expression and where it may fold; the error is the Load's
// if one of them stays unfolded
struct FoldCheck
{
    std::vector<FoldCandidate> candidates;
    unsigned from = 0;
    unsigned to = 0;
    // The numbers in [from, to) that are no tuple-slice subscripts, in order
    std::vector<FoldMark> marks;
    bool strictUndefined = false;
    ParseError error;
};

// How a fold check ends (FoldCheckPasses)
enum class FoldCheckResult : std::uint8_t
{
    Passes,
    // A tuple-slice subscript stays unfolded: the error is the check's
    Unfolded,
    // An integer past MaxIntStrDigits digits stays in the compiled code: Jinja2's repr() of it
    // raises ValueError (docs/tasks/0564)
    StrDigits,
    // With StrictUndefined and no tuple-slice subscript: the optimizer's bool() or str() of a
    // strict undefined raises UndefinedError (docs/tasks/0555)
    Undefined,
};

class RenderContext;
// Whether every subscript of the check is folded away, evaluated in `context`, a context on the
// sealed tree. With StrictUndefined, Jinja2's fold also fails the Load where its optimizer
// stringifies or tests the undefined such a subscript gives, so any candidate whose evaluation
// fails fails the check. An integer past MaxIntStrDigits digits that a fold gives and no fold
// around it takes away fails it too
// `budgeted`: the context runs under FoldBudget, whose limits make a candidate Impossible rather than fail the Load
FoldCheckResult FoldCheckPasses(const FoldCheck& check, RenderContext& context, bool budgeted = false);

// What the Load's fold checks may compute in a template without limits of its own (Settings::Limits): Jinja2 folds
// constants when it compiles, even in code that never renders, so template text decides how long a Load takes. A
// candidate the budget stops does not fold, which Jinja2's optimizer would only after computing it (docs/tasks/0569).
// Each check (a root) has a few steps of its own, enough for a literal; past them it draws on `Steps`, which are the
// whole Load's, so many cheap roots cannot add up either. Sizes and memory are each check's
struct FoldBudget
{
    static constexpr std::uint64_t Steps = std::uint64_t{ 1 } << 14;
    static constexpr std::uint64_t CheckSteps = 64;
    static constexpr std::uint64_t SequenceSize = std::uint64_t{ 1 } << 16;
    static constexpr std::uint64_t Memory = std::uint64_t{ 1 } << 24;
    // A product or power past it costs milliseconds (boost backend); Python's repr fails from 14286 bits on
    static constexpr std::size_t IntBits = std::size_t{ 1 } << 16;
    static const Settings::Limits& Limits();
};
// The ValueError CPython's repr() raises for an integer past MaxIntStrDigits digits, at `tok`
ParseError StrDigitsError(const Token& tok);
// FoldCheckResult::Undefined's error, at `tok`
ParseError StrictFoldError(const Token& tok);

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
    // The marks of the top-level expressions being parsed (ExpressionParser::AddFoldMark)
    std::vector<FoldMark>& FoldMarks();
    void DropFoldCandidates();
    // A fold candidate's node, which the tree may not hold: a step of a filter chain, made only
    // as a candidate, or an argument a filter drops (docs/tasks/0497). Seal checks them as roots
    // (TemplateParser::SealWithFoldNodes)
    void AddFoldNode(NodeRef<Expression> node);
    // Null when there are none
    [[nodiscard]] const std::vector<NodeRef<Expression>>* GetFoldNodes() const { return m_fold ? FoldNodes() : nullptr; }

private:
    std::vector<Name> m_names;
    struct FoldState;
    struct FoldStateDeleter
    {
        void operator()(FoldState* state) const;
    };
    FoldState& Fold();
    [[nodiscard]] const std::vector<FoldCheck>* FoldChecks() const;
    [[nodiscard]] const std::vector<NodeRef<Expression>>* FoldNodes() const;
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
    // The filters of a chain `|a|b`. In an expression (m_chainOperand) each of its steps but the
    // last is a fold candidate too
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
    // `operand`, parsed since m_tupleSliceSubscripts was foldFrom, filtered by `chain`, the
    // filters of a chain before the next one, is a candidate of its own (docs/tasks/0497)
    void NoteChainStep(NodeRef<Expression> operand, NodeRef<ExpressionFilter> chain, unsigned foldFrom)
    {
        if (chain && operand && m_tupleSliceSubscripts != foldFrom)
        {
            NoteChainStepFold(operand, chain, foldFrom);
        }
    }
    // Out of line: only with a tuple-slice subscript
    void NoteChainStepFold(NodeRef<Expression> operand, NodeRef<ExpressionFilter> chain, unsigned foldFrom);
    // At the end of the top-level expression starting at `tok`: an error if one of its subscripts
    // cannot fold, else the check goes to the Load
    nonstd::expected<void, ParseError> FinishFoldRoot(const Token& tok, unsigned rootFrom);
    // At the end of ParseFullExpression, when its value made tuple-slice subscripts: the value
    // is a candidate, and a top-level expression's check is made (out of line: few do)
    nonstd::expected<void, ParseError> NoteFullExpressionFold(NodeRef<Expression> result, unsigned from, const Token& tok);
    // Counts a constant that is no tuple-slice subscript with them (FoldMark), where the parse is
    // in a top-level expression: the expressions around it become candidates. Out of line: rare
    void AddFoldMark(FoldMarkKind kind);
    // AddFoldMark for an integer literal: past MaxIntStrDigits digits, or near them, where an
    // operator may take it past. Most literals are small: inline, a test of its kind
    void NoteIntLiteral(const InternalValue& value)
    {
        if (value.Kind() == ValueKind::BigInt)
        {
            NoteBigIntLiteral(value);
        }
    }
    void NoteBigIntLiteral(const InternalValue& value);
    // AddFoldMark for an operator, filter or subscript `node` that may give what a mark watches,
    // if it may fold at all
    void NoteWatchedFold(NodeRef<Expression> node);
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
    // Since Parse began (or, inside a conditional's test, since the test began) a name was read; and a
    // conditional or an `and`/`or` was parsed whose test or left operand read none. An expression that
    // reads a name folds only by the side such a test picks, so Parse walks an expression for its fold
    // (PrepareFold) only when it read no name or has such a choice (docs/tasks/0505)
    bool m_readsName = false;
    bool m_foldChoice = false;
    // The names read so far, which tells whether the operands of an operator read one (ParseBinary)
    unsigned m_nameReads = 0;
    // What the chain ParseFilterExpression parses filters, in an expression, and
    // m_tupleSliceSubscripts before it (NoteChainStep); null for a filter tag's chain
    NodeRef<Expression> m_chainOperand;
    unsigned m_chainFoldFrom = 0;
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
