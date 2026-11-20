#include "expression_parser.h"
#include "undefined.h"

#include "engine_errors.h"
#include "error_handling.h"
#include "expression_evaluator.h"
#include "filters.h"
#include "internal_value.h"
#include "lexer.h"
#include "loop_attr.h"
#include "make_unexpected.h"
#include "name_resolver.h"
#include "node_arena.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "renderer.h"
#include "small_vector.h"
#include "testers.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/prompt.h>
#include <jinja2cpp/template_env.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{
inline namespace v2
{

ParseError::ParseError(const ParseError&) = default;

nonstd::unexpected_type<ParseError> MakeParseErrorMsg(ErrorCode code, Token tok, std::string message)
{
    return MakeUnexpected(ParseError{ code, tok, std::move(message) });
}

ParseError::ParseError(ParseError&& other) noexcept
    : errorCode(other.errorCode)
    , errorToken(std::move(other.errorToken))
    , relatedTokens(std::move(other.relatedTokens))
    , message(std::move(other.message))
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
    std::swap(message, error.message);

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
// A literal with a malformed escape sequence ends the run and is stored in invalid
InternalValue ParseAdjacentStrings(LexScanner& lexer, InternalValue value, std::optional<Token>& invalid)
{
    Token tok;
    while (lexer.EatIfEqual(Token::String, &tok))
    {
        // Copies the text so far: adjacent literals are rare, and value stays whole on a break
        auto next = TakeString(lexer.GetValue(tok));
        if (lexer.TakeLiteralError())
        {
            invalid = tok;
            break;
        }
        auto str = next ? TakeString(InternalValue(value)) : std::nullopt;
        if (!str || !next)
        {
            break;
        }

        // The literals of one template have one character type
        std::visit([&next](auto& text) { text += std::get<std::decay_t<decltype(text)>>(*next); }, *str);
        value = InternalValue(std::move(*str));
    }

    return value;
}

ExpressionParser::ExpressionParser(const Settings& settings, TemplateEnv* env, NodeArena& nodes, NameResolver& names, UnknownCallables& unknown)
    : m_env(env)
    , m_nodes(nodes)
    , m_names(names)
    , m_unknown(unknown)
    , m_trustedLiterals(settings.autoescapeMode == AutoescapeMode::Prompt)
    , m_strictUndefined(settings.undefinedPolicy == UndefinedPolicy::Strict)
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

namespace
{
// Whether the candidates of `check` that `counts` (by index) cover each of its subscripts, in
// one sweep over their ranges
template<typename Counts>
bool CoversEverySubscript(const FoldCheck& check, const Counts& counts)
{
    std::vector<int> starts(check.to - check.from + 1, 0);
    for (std::size_t index = 0; index != check.candidates.size(); ++index)
    {
        const auto& candidate = check.candidates[index];
        // Only a candidate inside the check's range counts: an index outside it would write
        // past `starts`
        if (candidate.from >= check.from && candidate.to <= check.to && candidate.from <= candidate.to && counts(index))
        {
            ++starts[candidate.from - check.from];
            --starts[candidate.to - check.from];
        }
    }
    int open = 0;
    for (unsigned subscript = 0; subscript != check.to - check.from; ++subscript)
    {
        open += starts[subscript];
        if (open <= 0)
        {
            return false;
        }
    }
    return true;
}

// Whether Jinja2's as_const of the node turns an exception into Impossible (FoldCandidate::failsLoad).
// A conditional's as_const lets out what its test's truth raises, which the Load's evaluation
// of it decides (FoldEvaluator::ConditionalOf)
bool FoldCatchesFailure(const NodeArena& nodes, NodeRef<Expression> node)
{
    switch (nodes[node].GetKind())
    {
    case NodeKind::UnaryExpr:
    case NodeKind::IsExpr:
    case NodeKind::InLiteralExpr:
    case NodeKind::ConstFormatExpr:
    case NodeKind::CompareExpr:
    case NodeKind::SubscriptExpr:
    case NodeKind::LoopAttrExpr:
    case NodeKind::FilteredExpr:
    case NodeKind::SliceExpr:
        return true;
    // `and`, `or` and `~` let out only the UndefinedError of bool() or str(), which the Load's evaluation of them
    // decides (FoldEvaluator::LetsUndefinedOut, docs/tasks/0546)
    case NodeKind::BinaryExpr:
    case NodeKind::ConcatExpr:
        return true;
    case NodeKind::FullExpr:
        return static_cast<bool>(nodes.Get<FullExpressionEvaluator>(node).GetTester());
    default:
        return false;
    }
}

// FoldCandidate::leadingOperands of `node`, whose PrepareFold gave `fold`
std::uint32_t LeadingOperands(const NodeArena& nodes, NodeRef<Expression> node, std::uint8_t fold)
{
    switch (nodes[node].GetKind())
    {
    case NodeKind::BinaryExpr:
    {
        const auto& binary = nodes.Get<BinaryExpression>(node);
        return binary.IsLogical() && binary.LeftFolds() ? 1 : 0;
    }
    case NodeKind::ConcatExpr:
    {
        // A `~` that folds is evaluated whole
        if (fold != 0)
        {
            return 0;
        }
        std::uint32_t count = 0;
        for (const auto operand : nodes[nodes.Get<ConcatExpression>(node).GetOperands()])
        {
            if (nodes[operand].PrepareFold(nodes) == 0)
            {
                break;
            }
            ++count;
        }
        return count;
    }
    case NodeKind::FullExpr:
        // The test of a conditional whose branches do not fold (FoldEvaluator::ConditionalOf)
        return fold == 0 && nodes.Get<FullExpressionEvaluator>(node).GetTester() ? 1 : 0;
    default:
        return 0;
    }
}
} // namespace

ExpressionParser::ParseResult<NodeRef<IRendererBase>> ExpressionParser::Parse(LexScanner& lexer, bool isVolatile)
{
    m_volatile = isVolatile;
    // The tokens outlive the parse: no copy
    const auto& startTok = lexer.PeekNextToken();
    // The output is the top-level expression (FoldCheck): the expressions in it are none
    m_outputRoot = true;
    m_readsName = false;
    m_foldChoice = false;
    const unsigned foldFrom = m_tupleSliceSubscripts;
    auto evaluator = ParseTupleOrExpression(lexer);
    if (!evaluator)
    {
        return MakeUnexpected(evaluator.error());
    }
    // A subscript by a tuple of slices compiles where Jinja2 folds an expression around it, or
    // the output's whole value (FoldCheck); printing the strict undefined it folds to fails
    if (m_tupleSliceSubscripts != foldFrom)
    {
        if (m_strictUndefined && &m_nodes[*evaluator] == &m_nodes[m_tupleSliceValue])
        {
            return MakeParseError(ErrorCode::ExpectedSquareBracket, startTok);
        }
        AddFoldCandidate(*evaluator, foldFrom, true);
        if (auto finished = FinishFoldRoot(startTok, foldFrom); !finished)
        {
            return MakeUnexpected(finished.error());
        }
    }

    const auto& tok = lexer.NextToken();
    if (tok != Token::Eof)
    {
        auto tok1 = tok;
        tok1.type = Token::Eof;

        return MakeParseError(ErrorCode::ExpectedToken, tok, { tok1 });
    }

    // An expression that reads a name folds only by the side a conditional or `and`/`or` picks (m_foldChoice):
    // most `{{ }}` then skip the walk (PrepareFold), which costs a Load of expression-heavy templates about 3%
    return MakeExpressionRenderer(m_nodes, *evaluator, m_finalize, m_volatile, !m_readsName || m_foldChoice);
}

struct UnknownCallables::FoldState
{
    std::vector<FoldCheck> checks;
    std::vector<FoldCandidate> candidates;
    std::vector<NodeRef<Expression>> nodes;
};

void UnknownCallables::FoldStateDeleter::operator()(FoldState* state) const
{
    std::default_delete<FoldState>()(state);
}

UnknownCallables::FoldState& UnknownCallables::Fold()
{
    if (!m_fold)
    {
        m_fold.reset(new FoldState());
    }
    return *m_fold;
}

void UnknownCallables::AddFoldCheck(FoldCheck check)
{
    Fold().checks.push_back(std::move(check));
}

const std::vector<FoldCheck>* UnknownCallables::FoldChecks() const
{
    return !m_fold->checks.empty() ? &m_fold->checks : nullptr;
}

std::vector<FoldCandidate>& UnknownCallables::FoldCandidates()
{
    return Fold().candidates;
}

void UnknownCallables::AddFoldNode(NodeRef<Expression> node)
{
    Fold().nodes.push_back(node);
}

const std::vector<NodeRef<Expression>>* UnknownCallables::FoldNodes() const
{
    return !m_fold->nodes.empty() ? &m_fold->nodes : nullptr;
}

void UnknownCallables::DropFoldCandidates()
{
    if (m_fold)
    {
        m_fold->candidates.clear();
    }
}

void ExpressionParser::AddFoldCandidate(NodeRef<Expression> node, unsigned from, bool output)
{
    if (!InFoldRoot())
    {
        return;
    }
    const auto fold = m_nodes[node].PrepareFold(m_nodes);
    const auto leading = m_strictUndefined && !output ? LeadingOperands(m_nodes, node, fold) : 0;
    if (fold != 0 || leading != 0)
    {
        m_unknown.FoldCandidates().push_back(FoldCandidate{ node, from, m_tupleSliceSubscripts, fold, output, output || !FoldCatchesFailure(m_nodes, node), leading });
        // The tree may not hold it: a filter drops the arguments it does not take
        m_unknown.AddFoldNode(node);
    }
}

JINJA2CPP_NOINLINE nonstd::expected<void, ParseError> ExpressionParser::NoteFullExpressionFold(NodeRef<Expression> result, unsigned from, const Token& tok)
{
    // A statement's expression is a top-level one (FoldCheck), unless it is an output's
    const bool isFoldRoot = m_depth == 1 && !m_outputRoot && !m_inElseChain;
    AddFoldCandidate(result, from, isFoldRoot && m_outputExpressions);
    if (isFoldRoot)
    {
        return FinishFoldRoot(tok, from);
    }
    return {};
}

nonstd::expected<void, ParseError> ExpressionParser::FinishFoldRoot(const Token& tok, unsigned rootFrom)
{
    FoldCheck check;
    auto& candidates = m_unknown.FoldCandidates();
    // Only this root's: a root of this parser that failed left its own, of lower indices; the
    // first subscript of a parser drops those of the others (ParseSubscript)
    candidates.erase(std::remove_if(candidates.begin(),
                                    candidates.end(),
                                    [this, rootFrom](const FoldCandidate& candidate) { return candidate.from < rootFrom || candidate.to > m_tupleSliceSubscripts; }),
                     candidates.end());
    check.candidates = std::move(candidates);
    candidates.clear();
    check.from = rootFrom;
    check.to = m_tupleSliceSubscripts;
    if (!CoversEverySubscript(check, [](std::size_t /*index*/) { return true; }))
    {
        return MakeParseError(ErrorCode::ExpectedSquareBracket, tok);
    }
    check.strictUndefined = m_strictUndefined;
    check.error = ParseError(ErrorCode::ExpectedSquareBracket, tok);
    m_unknown.AddFoldCheck(std::move(check));
    return {};
}

bool ExpressionParser::AddOperator()
{
    return ++m_operators <= detail::t_parseLimits.maxExpressionOperators && !StackNearlyExhausted();
}


ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseFullExpression(LexScanner& lexer, bool includeIfPart)
{
    // Every nested expression (brackets, call arguments, subscripts, filter arguments, the
    // else branch of a conditional) starts here
    DepthGuard depthGuard(m_depth);
    if (depthGuard.Exceeds(detail::t_parseLimits.maxExpressionDepth) || StackNearlyExhausted())
    {
        return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
    }
    LexScanner::StateSaver saver(lexer);
    const auto unknownMark = m_unknown.Mark();
    // A statement's expression is a top-level one (FoldCheck); an output's is Parse's
    const unsigned foldFrom = m_tupleSliceSubscripts;

    auto value = ParseBinary(lexer, Precedence::Or);
    if (!value)
    {
        m_unknown.DeferSince(unknownMark);
        return MakeUnexpected(value.error());
    }

    // Only an inline `if` needs the wrapper: every compound node checks the stack itself.
    // Python's parse_condexpr loops: `a if b if c` is `(a if b) if c` (docs/tasks/0447); an
    // `else` takes the rest of the chain, so only else-less conditionals reach the next round
    NodeRef<Expression> result = *value;
    // The line Jinja2 reports is the one the conditional's value starts on: its first token,
    // which ParseBinary took; for a later round, the `if` that starts it
    size_t valueOffset = saver.m_state.m_cur->range.startOffset;
    bool chained = false;
    while (includeIfPart && lexer.PeekNextToken().keyword == Keyword::If)
    {
        const size_t ifOffset = lexer.PeekNextToken().range.startOffset;
        lexer.EatToken();
        if (chained)
        {
            if (!AddOperator())
            {
                m_unknown.DeferSince(unknownMark);
                return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
            }
            valueOffset = ifOffset;
        }
        auto ifExpr = ParseIfExpression(lexer, valueOffset);
        // Jinja2 checks the filters and tests of an inline if, its value among them, when they run
        m_unknown.DeferSince(unknownMark);
        if (!ifExpr)
        {
            return MakeUnexpected(ifExpr.error());
        }
        auto evaluator = m_nodes.Make<FullExpressionEvaluator>();
        m_nodes[evaluator].SetExpression(result);
        m_nodes[evaluator].SetTester(*ifExpr);
        result = NodeRef<Expression>(evaluator);
        chained = true;
    }

    if (m_tupleSliceSubscripts != foldFrom)
    {
        if (auto folded = NoteFullExpressionFold(result, foldFrom, *saver.m_state.m_cur); !folded)
        {
            return MakeUnexpected(folded.error());
        }
    }
    saver.Commit();
    return result;
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

// The binary operator the next token starts; Precedence::None if it starts none. Inlined:
// GCC's budget for this file left it out of line once, adding 1% to Load (docs/tasks/0216)
JINJA2CPP_ALWAYS_INLINE BinaryOperator PeekBinaryOperator(LexScanner& lexer)
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

namespace
{
// One `~` run as one node, emptying `operands`. Kept out of ParseBinary: inlined there it grew
// the function past what the compiler inlines into it, which every expression paid for at Load
JINJA2CPP_NOINLINE_INLINE NodeRef<Expression> MakeConcat(NodeArena& nodes, SmallVectorBase<NodeRef<Expression>>& operands)
{
    // The pure operands after the last one that is not are read in place
    const auto lastImpure = std::find_if(operands.rbegin(), operands.rend(), [&nodes](const NodeRef<Expression>& operand) { return !nodes[operand].IsPure(nodes); });
    const auto lendFrom = static_cast<uint32_t>(operands.rend() - lastImpure);
    const auto node = nodes.Make<ConcatExpression>(nodes.MakeSpan(operands), lendFrom);
    // Jinja2 folds a chain of foldable operands into one str, also where an operand folds by
    // the side a foldable test picks (docs/tasks/0307)
    if (const auto fold = nodes[node].PrepareFold(nodes); fold == FoldAlways)
    {
        nodes[node].SetFolded();
    }
    else if (fold == FoldByChoice)
    {
        nodes[node].SetFoldedByChoice();
    }
    operands.clear();
    return node;
}
} // namespace

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseBinary(LexScanner& lexer, Precedence minPrecedence)
{
    // The operators of one call combine like the old one-function-per-level chains: each
    // right operand starts counting from where this call started (OperatorChain)
    OperatorChain chain(m_operators);
    // The `~` of one call make one run, as Jinja2's Concat node: the operators around `~`
    // bind looser (a new call parses `a ~ b` on their right) or tighter (a ~ b * c), so the
    // operands of a run are always next to each other. Its operands are one flat node and
    // count as one operator
    SmallVector<NodeRef<Expression>, 4> concat;
    // Jinja2's optimizer folds each operator node made here too (docs/tasks/0484)
    const unsigned foldFrom = m_tupleSliceSubscripts;
    auto left = minPrecedence <= Precedence::Not && lexer.PeekNextToken().keyword == Keyword::LogicalNot ? ParseLogicalNot(lexer)
                                                                                                         : ParseUnaryPlusMinus(lexer);
    while (left)
    {
        const auto op = PeekBinaryOperator(lexer);
        if (op.precedence < minPrecedence)
        {
            break;
        }
        if (op.precedence != Precedence::Concat && !concat.empty())
        {
            *left = MakeConcat(m_nodes, concat);
            NoteFoldCandidate(*left, foldFrom);
        }
        if (op.precedence == Precedence::Compare)
        {
            // Every path assigns `left`, the one value returned, so the result is never moved out
            left = ParseComparisons(lexer, *left, op.operation, op.negated);
            if (left)
            {
                NoteFoldCandidate(*left, foldFrom);
            }
            continue;
        }

        const auto& tok = lexer.NextToken();
        chain.BeforeRight();
        // The left operand may fold only if it read no name, or by a choice already noted (m_foldChoice)
        m_foldChoice = m_foldChoice || (!m_readsName && (op.operation == BinaryExpression::LogicalAnd || op.operation == BinaryExpression::LogicalOr));
        // Every binary operator is left-associative, '**' included: Jinja2 reads 2 ** 3 ** 2 as 64
        auto right = op.precedence == Precedence::Pow ? ParseUnaryPlusMinus(lexer) : ParseBinary(lexer, Tighter(op.precedence));
        if (!right)
        {
            left = std::move(right);
        }
        // The operands after the first `~` of a run join it flat: the run is one operator
        else if (op.precedence == Precedence::Concat && !concat.empty() ? !chain.AfterRightFlat(detail::t_parseLimits.maxExpressionOperators)
                                                                        : !chain.AfterRight(detail::t_parseLimits.maxExpressionOperators))
        {
            left = MakeParseError(ErrorCode::RecursionLimitExceeded, tok);
        }
        else if (op.precedence == Precedence::Concat)
        {
            if (concat.empty())
            {
                concat.push_back(*left);
            }
            concat.push_back(*right);
        }
        else
        {
            *left = NodeRef<Expression>(BinaryExpression::Make(m_nodes, op.operation, *left, *right));
            NoteFoldCandidate(*left, foldFrom);
        }
    }

    if (left && !concat.empty())
    {
        *left = MakeConcat(m_nodes, concat);
        NoteFoldCandidate(*left, foldFrom);
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
    const unsigned foldFrom = m_tupleSliceSubscripts;
    auto expr = ParseBinary(lexer, Precedence::Not);
    if (!expr)
    {
        return expr;
    }

    const NodeRef<Expression> result = m_nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, *expr);
    NoteFoldCandidate(result, foldFrom);
    return result;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseComparisons(LexScanner& lexer,
                                                                                      NodeRef<Expression> left,
                                                                                      BinaryExpression::Operation operation,
                                                                                      bool negated)
{
    // Most chains are one comparison, which becomes a binary node: no heap list for it
    SmallVector<CompareExpression::Operand, 2> operands;
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

namespace
{
// Whether the token after an operand starts a postfix operator, a filter or a test. Most
// operands have none: their result is returned as it is
bool StartsOperandSuffix(const Token& tok)
{
    return tok == '.' || tok == '[' || tok == '(' || tok == '|' || tok.keyword == Keyword::Is;
}
} // namespace

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseUnaryPlusMinus(LexScanner& lexer, bool withFilter)
{
    const auto& tok = lexer.PeekNextToken();
    if (tok == '+' || tok == '-')
    {
        const unsigned foldFrom = m_tupleSliceSubscripts;
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
        NodeRef<Expression> unary =
            m_nodes.Make<UnaryExpression>(tok == '+' ? UnaryExpression::UnaryPlus : UnaryExpression::UnaryMinus, *subExpr);
        NoteFoldCandidate(unary, foldFrom);
        if (!StartsOperandSuffix(lexer.PeekNextToken()))
        {
            return unary;
        }
        return ParseOperandSuffix(lexer, unary, withFilter, foldFrom);
    }

    return ParseValueExpression(lexer, withFilter);
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseOperandSuffix(LexScanner& lexer,
                                                                                        NodeRef<Expression> operand,
                                                                                        bool withFilter,
                                                                                        unsigned foldFrom)
{
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
        return ParseFiltersAndTests(lexer, operand, foldFrom);
    }

    return operand;
}

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseValueExpression(LexScanner& lexer, bool withFilter)
{
    // The operand's subscripts are the chain's too (ParseFiltersAndTests)
    const unsigned foldFrom = m_tupleSliceSubscripts;
    const auto& tok = lexer.NextToken();

    NodeRef<Expression> value;
    switch (tok.type)
    {
    case Token::Identifier:
    {
        auto kwType = tok.keyword;
        if (kwType == Keyword::Is || kwType == Keyword::In || kwType == Keyword::If || kwType == Keyword::Else)
        {
            return MakeParseError(ErrorCode::UnexpectedToken, tok);
        }

        // The name goes straight from the source to the arena's symbol table
        const auto name = lexer.GetAsView(tok);
        if (name == "self")
        {
            m_nodes.NoteSelfUse(NodeArena::SelfUse::Read);
            value = SelfRefExpression::Make(m_nodes);
            break;
        }
        auto ref = ValueRefExpression::Make(m_nodes, name);
        m_readsName = true;
        m_names.AddUse(ref);
        if (m_names.ReadsOpen())
        {
            m_names.AddReadOf(m_nodes, ref);
        }
        value = ref;
        break;
    }
    case Token::IntegerNum:
    case Token::FloatNum:
        value = MakeConstant(m_nodes, lexer.GetValue(tok));
        break;
    case Token::String:
    {
        std::optional<Token> invalid;
        auto first = lexer.GetValue(tok);
        if (lexer.TakeLiteralError())
        {
            invalid = tok;
        }
        else
        {
            auto literal = ParseAdjacentStrings(lexer, std::move(first), invalid);
            if (m_trustedLiterals)
            {
                literal.SetMarkup();
            }
            value = MakeConstant(m_nodes, std::move(literal));
        }
        if (invalid)
        {
            return MakeParseError(ErrorCode::UnexpectedToken, *invalid);
        }
        break;
    }
    case Token::True:
        value = MakeConstant(m_nodes, InternalValue(true));
        break;
    case Token::False:
        value = MakeConstant(m_nodes, InternalValue(false));
        break;
    case Token::None:
        value = MakeConstant(m_nodes, InternalValue(NoneValue()));
        break;
    case '(':
    case '[':
    case '{':
    {
        auto nested = tok == '(' ? ParseBracedExpressionOrTuple(lexer) : tok == '[' ? ParseTuple(lexer)
                                                                                    : ParseDictionary(lexer);
        if (!nested)
        {
            return nested;
        }
        value = *nested;
        break;
    }
    default:
        return MakeParseError(ErrorCode::UnexpectedToken, tok);
    }

    // Most operands have neither a postfix nor a filter: their node is the result
    if (!StartsOperandSuffix(lexer.PeekNextToken()))
    {
        return value;
    }
    return ParseOperandSuffix(lexer, value, withFilter, foldFrom);
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

namespace
{
// Jinja2's parse_filter and parse_test read a dotted name, `x|ns.f`, as one name (docs/tasks/0465):
// appends each `.name` that follows to `name`. A name after the dot may be any of Jinja2's names,
// `true` and `none` among them. `nameTok` is widened to the whole name, which an unknown name's
// error quotes
JINJA2CPP_NOINLINE_INLINE nonstd::expected<void, ParseError> ReadDottedName(LexScanner& lexer, std::string& name, std::size_t& nameEnd)
{
    while (lexer.EatIfEqual('.'))
    {
        const auto& tok = lexer.NextToken();
        if (tok != Token::Identifier && tok != Token::True && tok != Token::False && tok != Token::None)
        {
            return MakeParseError(ErrorCode::ExpectedIdentifier, tok);
        }
        name += '.';
        name += lexer.GetAsString(tok);
        nameEnd = tok.range.endOffset;
    }
    return {};
}
} // namespace

ExpressionParser::ParseResult<NodeRef<Expression>> ExpressionParser::ParseFiltersAndTests(LexScanner& lexer,
                                                                                          NodeRef<Expression> valueRef,
                                                                                          unsigned foldFrom)
{
    ParseResult<NodeRef<Expression>> result = valueRef;
    bool stepped = false;
    while (result)
    {
        const auto& opTok = lexer.PeekNextToken();
        // Each step of the chain is a candidate, not only the chain (docs/tasks/0497)
        if (stepped && (opTok == '|' || opTok == '(' || opTok.keyword == Keyword::Is))
        {
            NoteFoldCandidate(*result, foldFrom);
        }
        stepped = true;
        if (lexer.EatIfEqual('|'))
        {
            if (!AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, opTok);
            }
            const auto outerOperand = m_chainOperand;
            const auto outerFoldFrom = m_chainFoldFrom;
            m_chainOperand = *result;
            m_chainFoldFrom = foldFrom;
            auto filter = ParseFilterExpression(lexer);
            m_chainOperand = outerOperand;
            m_chainFoldFrom = outerFoldFrom;
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

    // Jinja2's optimizer folds each filter and test of a chain, its operand included (FoldCheck):
    // the whole chain is the last candidate, so that its steps are evaluated only where it does
    // not fold (FoldCheckPasses)
    if (result)
    {
        NoteFoldCandidate(*result, foldFrom);
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
    if (auto dotted = lexer.PeekNextToken() == '.' ? ReadDottedName(lexer, name, nameTok.range.endOffset) : nonstd::expected<void, ParseError>(); !dotted)
    {
        return MakeUnexpected(dotted.error());
    }

    CallArgsInfo params;
    const auto& argTok = lexer.PeekNextToken();
    if (lexer.EatIfEqual('('))
    {
        auto parsedParams = ParseCallArgs(lexer);
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
            auto arg = ParseValueExpression(lexer, false);
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
        bool unknown = false;
        if (params.dynArgs || params.dynKwargs)
        {
            // Bound to the test each time it runs (docs/tasks/0306)
            const auto test = CreateUnpackingTester(m_nodes, name, params, FindRegisteredTester(name), &unknown);
            result = m_nodes.Make<IsExpression>(valueRef, test, false);
        }
        else
        {
            result = IsExpression::Make(m_nodes, valueRef, name, params, FindRegisteredTester(name), &unknown, !m_volatile);
        }
        if (unknown)
        {
            m_unknown.Add(nameTok, true);
            if (m_names.ReadsOpen())
            {
                m_names.AddReadOfName(name);
            }
        }
    }
    catch (const TemplateRuntimeError&)
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
        // Python's {key: value}
        siblings.Next();
        auto keyTok = lexer.PeekNextToken();
        auto key = ParseFullExpression(lexer);
        if (!key)
        {
            return ReplaceErrorIfPossible(key, keyTok, ErrorCode::ExpectedExpression);
        }

        auto sepTok = lexer.NextToken();
        if (sepTok != Token::Colon)
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

    ParseResult<CallArgsInfo> params = ParseCallArgs(lexer);
    if (!params)
    {
        return MakeUnexpected(params.error());
    }

    result = m_nodes.Make<CallExpression>(m_nodes, valueRef, std::move(*params));

    return result;
}

namespace
{
enum class CallArgKind
{
    Positional,
    Keyword,
    Args,
    Kwargs
};

// `*expr` or `**expr`: one of each, and no `*` after `**`
std::optional<CallArgKind> ClassifyUnpacked(const Token& tok, const CallArgsInfo& params)
{
    if (tok == Token::MulMul)
    {
        return params.dynKwargs ? std::nullopt : std::optional(CallArgKind::Kwargs);
    }
    return params.dynKwargs || params.dynArgs ? std::nullopt : std::optional(CallArgKind::Args);
}

bool HasUnpacked(const CallArgsInfo& params)
{
    return params.dynArgs || params.dynKwargs;
}

bool HasUnpackedKeywords(const CallArgsInfo& params)
{
    return static_cast<bool>(params.dynKwargs);
}

// The kind of argument `tok` starts, as Jinja2's parse_call_args orders them: positional
// arguments, then keywords and one `*expr`, then one `**expr`. Nothing where it is out of that
// order ("invalid syntax for function call expression") or a keyword is repeated; a keyword's
// name goes to `name`
std::optional<CallArgKind> ClassifyCallArg(const Token& tok, LexScanner& lexer, const CallArgsInfo& params, std::string& name)
{
    if (tok == '*' || tok == Token::MulMul)
    {
        return ClassifyUnpacked(tok, params);
    }
    if (tok == Token::Identifier && lexer.PeekNextToken() == '=')
    {
        name = lexer.GetAsString(tok);
        // No keyword after **kwargs
        return HasUnpackedKeywords(params) || params.kwParams.count(name) != 0 ? std::nullopt : std::optional(CallArgKind::Keyword);
    }
    return !params.kwParams.empty() || HasUnpacked(params) ? std::nullopt : std::optional(CallArgKind::Positional);
}
} // namespace

// The arguments of a call, a filter or a test, `*args` and `**kwargs` included
ExpressionParser::ParseResult<CallArgsInfo> ExpressionParser::ParseCallArgs(LexScanner& lexer)
{
    CallArgsInfo result;

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
        const auto kind = ClassifyCallArg(tok, lexer, result, paramName);
        if (!kind)
        {
            return MakeParseError(ErrorCode::UnexpectedToken, tok);
        }
        if (*kind == CallArgKind::Keyword)
        {
            lexer.EatToken();
        }
        else if (*kind == CallArgKind::Positional)
        {
            lexer.ReturnToken();
        }

        auto valueExpr = ParseFullExpression(lexer);
        if (!valueExpr)
        {
            return MakeUnexpected(valueExpr.error());
        }
        if (*kind == CallArgKind::Positional)
        {
            result.posParams.push_back(*valueExpr);
        }
        else if (*kind == CallArgKind::Keyword)
        {
            result.kwParams[paramName] = *valueExpr;
        }
        else
        {
            (*kind == CallArgKind::Args ? result.dynArgs : result.dynKwargs) = *valueExpr;
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
    else if (auto value = tok == Token::IntegerNum || tok == Token::FloatNum ? lexer.GetValue(tok) : InternalValue();
             GetIf<int64_t>(&value) || value.Kind() == ValueKind::BigInt)
    {
        result.indexExpr = MakeConstant(nodes, std::move(value));
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
    // A tuple with a slice among its items, as `l[1:2, 3]` (docs/tasks/0445): whether every part
    // is foldable; the parts themselves are not kept
    bool isTupleOfSlices = false;
    bool foldable = true;
    // Of a tuple of slices: its items
    ArenaSpan<TupleSliceExpression::Item> tupleItems;
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

// Jinja2's parse_subscribed: one item, an expression or a slice [start:stop:step] with any part omitted
SubscriptParseResult<BracketSubscript> ParseSubscribed(ExpressionParser& parser, LexScanner& lexer, SiblingOperators& siblings)
{
    BracketSubscript result;
    if (!EndsSlicePart(lexer))
    {
        auto expr = ParseSubscriptPart(parser, lexer, siblings);
        if (!expr)
        {
            return MakeUnexpected(expr.error());
        }
        result.parts[0] = *expr;
    }
    if (lexer.EatIfEqual(':'))
    {
        auto tail = ParseSliceTail(parser, lexer, siblings, result);
        if (!tail)
        {
            return MakeUnexpected(tail.error());
        }
    }
    else if (!result.parts[0])
    {
        return MakeParseError(ErrorCode::ExpectedExpression, lexer.PeekNextToken());
    }
    return result;
}

// The rest of a subscript by a tuple after its first item, if any: `l[a, b]`, `l[]`, `l[1:2, 3]`
SubscriptParseResult<BracketSubscript> ParseSubscriptTuple(ExpressionParser& parser, LexScanner& lexer, SiblingOperators& siblings, const BracketSubscript& first)
{
    auto& nodes = parser.Nodes();
    std::vector<NodeRef<Expression>> items;
    bool hasSlice = first.isSlice;
    bool foldable = true;
    std::vector<TupleSliceExpression::Item> sliceItems;
    const auto addItem = [&nodes, &foldable, &items, &sliceItems](const BracketSubscript& item) {
        for (const auto& part : item.parts)
        {
            foldable = foldable && (!part || nodes[part].IsFoldable(nodes));
        }
        items.push_back(item.parts[0]);
        sliceItems.push_back(TupleSliceExpression::Item{ { item.parts[0], item.parts[1], item.parts[2] }, item.isSlice });
    };
    if (first.parts[0] || first.isSlice)
    {
        addItem(first);
    }
    while (lexer.EatIfEqual(','))
    {
        // Jinja2 has no trailing comma in a subscript: `l[1,]` is a syntax error
        if (lexer.PeekNextToken() == ']')
        {
            return MakeParseError(ErrorCode::ExpectedExpression, lexer.PeekNextToken());
        }
        auto item = ParseSubscribed(parser, lexer, siblings);
        if (!item)
        {
            return MakeUnexpected(item.error());
        }
        hasSlice = hasSlice || item->isSlice;
        addItem(*item);
    }
    BracketSubscript result;
    if (hasSlice)
    {
        result.isTupleOfSlices = true;
        result.foldable = foldable;
        result.tupleItems = nodes.MakeSpan(sliceItems);
    }
    else
    {
        result.parts[0] = nodes.Make<TupleCreator>(nodes.MakeSpan(items), true);
    }
    return result;
}

// Jinja2's parse_subscript: an index, a slice, or a tuple of them. l[a, b] and l[] index with a
// tuple; a tuple with a slice among its items (`l[1:2, 3]`, Python's extended slice) is only
// told apart, as the caller folds or rejects it (docs/tasks/0445)
SubscriptParseResult<BracketSubscript> ParseBracketSubscript(ExpressionParser& parser, LexScanner& lexer, SiblingOperators& siblings)
{
    BracketSubscript result;
    if (lexer.PeekNextToken() != ']')
    {
        auto first = ParseSubscribed(parser, lexer, siblings);
        if (!first)
        {
            return MakeUnexpected(first.error());
        }
        result = *first;
    }
    // l[a, b] and l[] index with a tuple, as in Jinja2
    if (lexer.PeekNextToken() == ',' || (!result.parts[0] && !result.isSlice && lexer.PeekNextToken() == ']'))
    {
        auto tuple = ParseSubscriptTuple(parser, lexer, siblings, result);
        if (!tuple)
        {
            return MakeUnexpected(tuple.error());
        }
        result = *tuple;
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
    // An attribute or item of the undefined a tuple of slices folds to is a plain subscript of
    // it: its evaluation fails as Jinja2's fold of it does, except under ChainableUndefined,
    // and a fold around it that does not evaluate it (`true or [1][:, :][0]`) still covers the
    // tuple of slices, which the Load decides (FoldCheckPasses, docs/tasks/0484)
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
        if (bracket->isTupleOfSlices)
        {
            // Jinja2 folds a subscript of a literal by constants through Environment.getitem,
            // which gives an undefined for the TypeError of a list, tuple or string and the
            // KeyError of a dict. Unfolded, the slice inside the tuple is invalid Python
            if (!InFoldRoot() || !bracket->foldable || !m_nodes[valueRef].IsFoldable(m_nodes))
            {
                return MakeParseError(ErrorCode::ExpectedSquareBracket, lexer.PeekNextToken());
            }
            // The candidates left now are those of another parser's root that failed
            if (m_tupleSliceSubscripts++ == 0)
            {
                m_unknown.DropFoldCandidates();
            }
            const NodeRef<Expression> undefined = m_nodes.Make<TupleSliceExpression>(valueRef, bracket->tupleItems);
            m_tupleSliceValue = undefined;
            return undefined;
        }
        if (bracket->isSlice)
        {
            // Whether Jinja2 folds it decides what a slice of a value without items gives
            const auto slice = m_nodes.Make<SliceExpression>(valueRef, parts[0], parts[1], parts[2]);
            m_nodes[slice].PrepareFold(m_nodes);
            return slice;
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
    // (a sandbox policy is asked about the attributes of loop by name, so the sandbox has no resolved attribute)
    if (!attrName.empty() && !detail::t_parseLimits.sandboxed && m_nodes[value].GetKind() == NodeKind::NameRef && m_nodes.Get<ValueRefExpression>(value).GetName(m_nodes) == "loop")
    {
        if (const auto attr = FindLoopAttr(attrName); attr != LoopAttr::None)
        {
            return m_nodes.Make<LoopAttrExpression>(value, attr);
        }
    }
    return m_nodes.Make<SubscriptExpression>(value);
}

void ExpressionParser::NoteChainStepFold(NodeRef<Expression> operand, NodeRef<ExpressionFilter> chain, unsigned foldFrom)
{
    if (!InFoldRoot())
    {
        return;
    }
    const NodeRef<Expression> step = m_nodes.Make<FilteredExpression>(m_nodes, operand, chain);
    // A root for Seal even where it is no candidate (UnknownCallables::AddFoldNode)
    m_unknown.AddFoldNode(step);
    NoteFoldCandidate(step, foldFrom);
}

ExpressionParser::ParseResult<NodeRef<ExpressionFilter>> ExpressionParser::ParseFilterExpression(LexScanner& lexer)
{
    NodeRef<ExpressionFilter> result;

    const auto& startTok = lexer.PeekNextToken();
    try
    {
        do
        {
            // Jinja2's optimizer folds each filter of the chain on its own: a step before the
            // last folds even where the next one does not (docs/tasks/0497). The step's filter
            // nodes are shared with the chain, which does not change them; rare: only with a
            // tuple-slice subscript in the steps so far
            NoteChainStep(m_chainOperand, result, m_chainFoldFrom);
            const auto& tok = lexer.NextToken();
            if (tok != Token::Identifier)
            {
                return MakeParseError(ErrorCode::ExpectedIdentifier, tok);
            }

            std::string name = lexer.GetAsString(tok);
            auto nameEnd = tok.range.endOffset;
            // Rare: out of line
            if (auto dotted = lexer.PeekNextToken() == '.' ? ReadDottedName(lexer, name, nameEnd) : nonstd::expected<void, ParseError>(); !dotted)
            {
                return MakeUnexpected(dotted.error());
            }
            const bool hasArgs = lexer.NextToken() == '(';
            if (!hasArgs)
            {
                lexer.ReturnToken();
            }
            // Made in place: assigning an expected swaps the arguments
            auto params = hasArgs ? ParseCallArgs(lexer) : ParseResult<CallArgsInfo>();

            if (!params)
            {
                return MakeUnexpected(params.error());
            }

            bool unknown = false;
            NodeRef<ExpressionFilter> filter;
            if (params->dynArgs || params->dynKwargs)
            {
                // Bound to the filter each time it runs (docs/tasks/0306); such a call is not folded
                const auto unpacking = CreateUnpackingFilter(m_nodes, name, *params, FindRegisteredFilter(name), &unknown);
                filter = m_nodes.Make<ExpressionFilter>(unpacking, ArenaText(), false);
            }
            else
            {
                filter = ExpressionFilter::Make(m_nodes, name, *params, FindRegisteredFilter(name), &unknown, !m_volatile);
            }
            if (unknown)
            {
                // Quoting the whole dotted name
                auto nameTok = tok;
                nameTok.range.endOffset = nameEnd;
                m_unknown.Add(nameTok, false);
                if (m_names.ReadsOpen())
                {
                    m_names.AddReadOfName(name);
                }
            }
            else if (m_names.ReadsOpen())
            {
                NoteNamedCallables(name, *params);
            }
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
    catch (const TemplateRuntimeError&)
    {
        return MakeParseError(ErrorCode::UnexpectedException, startTok);
    }
    return result;
}

ExpressionParser::ParseResult<NodeRef<IfExpression>> ExpressionParser::ParseIfExpression(LexScanner& lexer, size_t valueOffset)
{
    NodeRef<IfExpression> result;
    const auto& startTok = lexer.PeekNextToken();

    try
    {
        const unsigned foldFrom = m_tupleSliceSubscripts;
        // The value before `if` may read a name the test does not: the test's own reads decide (m_foldChoice)
        const bool valueReadName = std::exchange(m_readsName, false);
        auto testExpr = ParseBinary(lexer, Precedence::Or);
        if (!testExpr)
        {
            return MakeUnexpected(testExpr.error());
        }
        m_foldChoice = m_foldChoice || !m_readsName;
        m_readsName = m_readsName || valueReadName;
        // Folded by the optimizer whatever the conditional does (FoldCheck)
        NoteFoldCandidate(*testExpr, foldFrom);

        ParseResult<NodeRef<Expression>> altValue;
        if (lexer.PeekNextToken().keyword == Keyword::Else)
        {
            lexer.EatToken();
            // 'a if x else b if y else c' chains like an operator; Python reads it in a loop
            if (!AddOperator())
            {
                return MakeParseError(ErrorCode::RecursionLimitExceeded, lexer.PeekNextToken());
            }
            // Not counted as nested, and still in the expression's root, if it is one (IsFoldRoot)
            --m_depth;
            const bool inElse = m_inElseChain;
            m_inElseChain = true;
            auto value = ParseFullExpression(lexer);
            m_inElseChain = inElse;
            ++m_depth;
            if (!value)
            {
                return MakeUnexpected(value.error());
            }
            altValue = *value;
        }

        // Without `else`, Jinja2's cond_expr_undefined: a jinja2.Undefined whatever the environment's
        // undefined, which prints empty and whose attribute, item or call fails. Made here, so
        // that a render copies it
        const bool noElse = !*altValue;
        if (noElse)
        {
            *altValue = m_nodes.Make<ConstantExpression>(MakeCondExprUndefined(lexer.LineOf(valueOffset)));
        }
        result = m_nodes.Make<IfExpression>(*testExpr, *altValue, noElse);
    }
    catch (const ParseError& error)
    {
        return MakeUnexpected(error);
    }
    catch (const TemplateRuntimeError&)
    {
        return MakeParseError(ErrorCode::UnexpectedException, startTok);
    }

    return result;
}

namespace
{
// Jinja2's has_safe_repr, which the optimizer asks of a folded value before it puts it in the
// code: None, bools, numbers, strings, and lists, tuples and dicts of them; not an undefined
bool HasSafeRepr(const InternalValue& value)
{
    // Repeated lists share their items, so a small value can have very many: under limits
    // each visit is a render step, which bounds the walk (RecursionLimitError near the
    // stack's end, both failing the Load)
    CheckStack();
    switch (value.Kind())
    {
    case ValueKind::None:
    case ValueKind::Bool:
    case ValueKind::String:
    case ValueKind::Int:
    case ValueKind::Double:
    case ValueKind::BigInt:
        return true;
    case ValueKind::List:
    {
        const auto list = AsList(value);
        if (!list || list->GetIteratorKind() != IteratorKind::None)
        {
            return false;
        }
        bool safe = true;
        list->ForEach([&safe](const InternalValue& item) {
            safe = HasSafeRepr(item);
            return safe;
        });
        return safe;
    }
    case ValueKind::Map:
    {
        const auto map = AsMap(value);
        if (!map || map->IsHost() || map->IsNamespace())
        {
            return false;
        }
        const auto entries = map->GetEntries();
        return std::all_of(entries.begin(), entries.end(), [](const KeyValuePair& entry) { return HasSafeRepr(entry.key) && HasSafeRepr(entry.value); });
    }
    default:
        return false;
    }
}

enum class FoldOutcome : std::uint8_t
{
    Folds,
    DoesNotFold,
    // Its evaluation failed: what Jinja2's as_const turns into Impossible or, depending on the
    // candidate's kind, lets out as UndefinedError (FoldCandidate::failsLoad)
    Fails,
    // Its evaluation let out an UndefinedError whatever its kind: the strict undefined a
    // conditional tests (docs/tasks/0527)
    FailsLoad,
};

// What a candidate evaluates to, or that the evaluation failed with an error Jinja2's fold
// turns into Impossible or UndefinedError; a limit of the render, a recursion limit or a bug
// fails the Load as it is
struct FoldValue
{
    InternalValue value;
    bool failed = false;
};

// A filter's value error the render reports as an ErrorInfo (RendererCallback::ThrowRuntimeError): `|abs` of
// a strict undefined. Jinja2's Filter.as_const turns it into Impossible (docs/tasks/0536)
template<typename CharT>
bool IsValueError(const BasicErrorInfo<CharT>& error)
{
    return error.GetCode() == ErrorCode::InvalidValueType;
}

template<typename Evaluate>
FoldValue TryFoldValue(Evaluate&& evaluate)
{
    try
    {
        return FoldValue{ std::forward<Evaluate>(evaluate)(), false };
    }
    catch (const TemplateRuntimeError&)
    {
        return FoldValue{ InternalValue(), true };
    }
    catch (const UndefinedError&)
    {
        return FoldValue{ InternalValue(), true };
    }
    catch (const IntegerOutOfRangeError&)
    {
        return FoldValue{ InternalValue(), true };
    }
    catch (const ErrorInfo& error)
    {
        if (!IsValueError(error))
        {
            throw;
        }
        return FoldValue{ InternalValue(), true };
    }
    catch (const ErrorInfoW& error)
    {
        if (!IsValueError(error))
        {
            throw;
        }
        return FoldValue{ InternalValue(), true };
    }
}

// What bool() or str() of fails in Jinja2: a StrictUndefined (cond_expr_undefined, of an inline if without
// else, is a plain Undefined whatever the environment's)
bool IsStrictUndefined(const InternalValue& value)
{
    if (!value.IsUndefined())
    {
        return false;
    }
    if (const auto* info = GetUndefinedInfo(value))
    {
        return info->policy == UndefinedPolicy::Strict;
    }
    return true;
}

// The truth of a folded value; nothing when bool() of it fails
std::optional<bool> FoldedTruth(const FoldValue& value)
{
    if (value.failed)
    {
        return std::nullopt;
    }
    try
    {
        return ConvertToBool(value.value);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

// What the check reads of a value once the value itself is gone (FoldEvaluator::Release)
struct FoldSummary
{
    bool failed = false;
    bool undefined = false;
    bool strictUndefined = false;
    bool safeRepr = false;
    // bool() of it; nothing when that fails
    std::optional<bool> truth;
};

FoldSummary Summarize(const FoldValue& value, bool withRepr)
{
    FoldSummary summary;
    summary.failed = value.failed;
    if (!value.failed)
    {
        summary.undefined = value.value.IsUndefined();
        summary.strictUndefined = IsStrictUndefined(value.value);
        summary.truth = FoldedTruth(value);
        summary.safeRepr = withRepr && HasSafeRepr(value.value);
    }
    return summary;
}

// Evaluates the fold candidates of one check (FoldCheckPasses). A candidate whose operand is a candidate too is
// a step after it: a filter chain's `x|a`, `x|a|b`, ..., an operator chain's `x or a`, `(x or a) or b`, ..., and
// `not x`, `not not x`, a subscript `(x)[0]` and a test `(x) is y`. A step's value and whether it folds come from
// those of the step before it, kept from that step, so a chain of N filters, operators, subscripts or tests runs
// N of them, not N^2/2, and a host filter runs once (docs/tasks/0527, 0536, 0546). A kept value is dropped once
// every step after it has read it and its own candidate is done, so a chain holds about two values at a time
class FoldEvaluator
{
public:
    FoldEvaluator(const FoldCheck& check, RenderContext& context)
        : m_check(check)
        , m_context(context)
        , m_steps(check.candidates.size())
    {
        LinkSteps();
    }

    FoldOutcome Of(std::size_t index)
    {
        const auto outcome = OutcomeOf(index);
        Done(index);
        return outcome;
    }

    // A candidate inside one that folds, which needs no outcome of its own
    void Skip(std::size_t index) { Done(index); }

private:
    enum class StepKind : std::uint8_t
    {
        None,
        Filter,
        Binary,
        Unary,
        Subscript,
        Test
    };
    struct Step
    {
        // The candidate that is this step's operand (a filter step's chain without its last filter), if any
        std::size_t previous = NoStep;
        StepKind kind = StepKind::None;
        // Whether value is this step's, evaluated
        bool known = false;
        // Whether folds is this step's (FoldsAt)
        bool foldsKnown = false;
        bool folds = false;
        // Whether its candidate is done (Of or Skip), and whether its read of the previous step is
        bool done = false;
        bool readDone = false;
        // Whether summary is the value's, and whether value was dropped, leaving it
        bool summarized = false;
        bool released = false;
        // The steps after it that have not read its value yet
        std::uint32_t readers = 0;
        FoldValue value;
        FoldSummary summary;
    };
    static constexpr std::size_t NoStep = static_cast<std::size_t>(-1);

    FoldOutcome OutcomeOf(std::size_t index)
    {
        const auto& candidate = m_check.candidates[index];
        const auto& nodes = m_context.Nodes();
        // Not an output's: an output folds whole into str(value), which may fail where the
        // conditional's as_const does not (visit_Output)
        if (!candidate.output)
        {
            if (const auto full = nodes.As<FullExpressionEvaluator>(candidate.node); full && nodes[full].GetTester())
            {
                return ConditionalOf(nodes[full]);
            }
        }
        if (candidate.leadingOperands != 0 && LeadingOperandFails(index))
        {
            return FoldOutcome::FailsLoad;
        }
        if (candidate.fold == 0 || !FoldsAt(index))
        {
            return FoldOutcome::DoesNotFold;
        }
        const auto& result = SummaryOf(index);
        if (result.failed)
        {
            // An `and`, `or` or `~` lets out only the UndefinedError of its own bool() or str(), or of such an
            // operand's; the failure of any other operand is Impossible to it (docs/tasks/0546)
            if (m_check.strictUndefined && !candidate.output && LetsUndefinedOut(candidate.node))
            {
                return FoldOutcome::FailsLoad;
            }
            return FoldOutcome::Fails;
        }
        // An output folds into str(value), which fails for a strict undefined
        if (candidate.output)
        {
            return m_check.strictUndefined && result.undefined ? FoldOutcome::Fails : FoldOutcome::Folds;
        }
        return result.safeRepr ? FoldOutcome::Folds : FoldOutcome::DoesNotFold;
    }

    void LinkSteps()
    {
        // A step is recorded after the one before it, as its parse ends
        const auto& nodes = m_context.Nodes();
        std::vector<std::pair<const void*, std::size_t>> chains;
        const auto link = [this, &nodes](std::size_t index, NodeRef<Expression> operand, StepKind kind) {
            if (const auto found = m_candidateOf.find(&nodes[operand]); found != m_candidateOf.end())
            {
                m_steps[index].previous = found->second;
                m_steps[index].kind = kind;
            }
        };
        for (std::size_t index = 0; index != m_check.candidates.size(); ++index)
        {
            const auto node = m_check.candidates[index].node;
            switch (nodes[node].GetKind())
            {
            case NodeKind::FilteredExpr:
            {
                const auto& step = nodes.Get<FilteredExpression>(node);
                if (const auto parent = nodes[step.GetFilter()].GetParentFilter())
                {
                    const void* parentChain = &nodes[parent];
                    const auto found = std::find_if(chains.rbegin(), chains.rend(), [parentChain](const auto& chain) { return chain.first == parentChain; });
                    if (found != chains.rend() && &nodes[nodes.Get<FilteredExpression>(m_check.candidates[found->second].node).GetOperand()] == &nodes[step.GetOperand()])
                    {
                        m_steps[index].previous = found->second;
                        m_steps[index].kind = StepKind::Filter;
                    }
                }
                else
                {
                    // The chain's first filter: a step after its operand's candidate, `(x)|f`
                    link(index, step.GetOperand(), StepKind::Filter);
                }
                chains.emplace_back(&nodes[step.GetFilter()], index);
                break;
            }
            case NodeKind::BinaryExpr:
                link(index, nodes.Get<BinaryExpression>(node).GetLeftExpr(), StepKind::Binary);
                break;
            case NodeKind::UnaryExpr:
                link(index, nodes.Get<UnaryExpression>(node).GetOperand(), StepKind::Unary);
                break;
            // Not a LoopAttrExpression, whose value is the name `loop`, which never folds
            case NodeKind::SubscriptExpr:
                link(index, nodes.Get<SubscriptExpression>(node).GetValue(), StepKind::Subscript);
                break;
            case NodeKind::IsExpr:
                link(index, nodes.Get<IsExpression>(node).GetValue(), StepKind::Test);
                break;
            default:
                break;
            }
            if (const auto previous = m_steps[index].previous; previous != NoStep)
            {
                ++m_steps[previous].readers;
            }
            // The latest candidate of each node: an output and its non-output duplicate have the same value
            m_candidateOf[&nodes[node]] = index;
        }
    }

    // Whether the candidate folds in this render (Expression::IsFoldableAt); a step's from the step before it
    bool FoldsAt(std::size_t index)
    {
        if (StepFoldsAlways(index) || m_steps[index].foldsKnown)
        {
            return StepFoldsAlways(index) || m_steps[index].folds;
        }
        std::vector<std::size_t> pending{ index };
        for (auto previous = m_steps[index].previous; previous != NoStep && !StepFoldsAlways(previous) && !m_steps[previous].foldsKnown; previous = m_steps[previous].previous)
        {
            pending.push_back(previous);
        }
        while (!pending.empty())
        {
            const auto step = pending.back();
            pending.pop_back();
            m_steps[step].folds = StepFoldsAt(step);
            m_steps[step].foldsKnown = true;
        }
        return m_steps[index].folds;
    }

    [[nodiscard]] bool StepFoldsAlways(std::size_t index) const { return (m_check.candidates[index].fold & FoldAlways) != 0; }

    // FoldsAt of a step whose previous step's is known
    bool StepFoldsAt(std::size_t index)
    {
        const auto& nodes = m_context.Nodes();
        const auto node = m_check.candidates[index].node;
        const auto previous = m_steps[index].previous;
        const bool previousFolds = previous != NoStep && (StepFoldsAlways(previous) || m_steps[previous].folds);
        switch (m_steps[index].kind)
        {
        case StepKind::None:
            return nodes[node].IsFoldableAt(m_context);
        case StepKind::Filter:
        case StepKind::Unary:
            // The operand of both is the operand of the step before
            return previousFolds;
        case StepKind::Subscript:
            return previousFolds && nodes.Get<SubscriptExpression>(node).IndicesFoldAt(m_context);
        case StepKind::Test:
            return previousFolds && nodes.Get<IsExpression>(node).TestFolds();
        case StepKind::Binary:
        {
            // BinaryExpression::IsFoldableAt, with the left operand's fold and truth from its step
            const auto& binary = nodes.Get<BinaryExpression>(node);
            if (!previousFolds)
            {
                return false;
            }
            if (binary.IsLogical())
            {
                const auto truth = SummaryOf(previous).truth;
                if (!truth)
                {
                    return false;
                }
                if (*truth == binary.IsOr())
                {
                    return true;
                }
            }
            return binary.RightFoldsAt(m_context);
        }
        }
        return false;
    }

    // With StrictUndefined: an operand before the first that does not fold gives a strict undefined, whose bool()
    // or str() fails the compile (FoldCandidate::leadingOperands)
    bool LeadingOperandFails(std::size_t index)
    {
        const auto& candidate = m_check.candidates[index];
        const auto& nodes = m_context.Nodes();
        if (const auto binary = nodes.As<BinaryExpression>(candidate.node))
        {
            if (m_steps[index].kind == StepKind::Binary)
            {
                const auto previous = m_steps[index].previous;
                return FoldsAt(previous) && SummaryOf(previous).strictUndefined;
            }
            if (!nodes[binary].LeftFoldsAt(m_context))
            {
                return false;
            }
            return OperandSummary(nodes[binary].GetLeftExpr()).strictUndefined;
        }
        if (const auto concat = nodes.As<ConcatExpression>(candidate.node))
        {
            const auto operands = nodes[nodes[concat].GetOperands()];
            for (std::uint32_t idx = 0; idx != candidate.leadingOperands && idx != operands.size(); ++idx)
            {
                if (!nodes[operands[idx]].IsFoldableAt(m_context))
                {
                    return false;
                }
                const auto& operand = OperandSummary(operands[idx]);
                // A failure of its own is the Impossible that stops the join
                if (operand.failed)
                {
                    return false;
                }
                if (operand.strictUndefined)
                {
                    return true;
                }
            }
        }
        return false;
    }

    // With StrictUndefined, of a node Jinja2's as_const evaluates (one that folds, or an operand it reaches):
    // whether the UndefinedError of bool() or str() of a strict undefined leaves its as_const. Only `and`, `or`,
    // `~` and a conditional let it out, where their own operand is such a value or lets it out; any other node's
    // as_const turns it into Impossible, but the optimizer folds the operands first, so one of them that lets
    // it out fails the compile all the same. Memoized: a failing step's operand is the step before it
    bool LetsUndefinedOut(NodeRef<Expression> expr)
    {
        if (!expr)
        {
            return false;
        }
        const auto& nodes = m_context.Nodes();
        const void* key = &nodes[expr];
        if (const auto found = m_letsOut.find(key); found != m_letsOut.end())
        {
            return found->second;
        }
        CheckStack();
        const bool result = ComputeLetsUndefinedOut(expr);
        m_letsOut[key] = result;
        return result;
    }

    bool ComputeLetsUndefinedOut(NodeRef<Expression> expr)
    {
        const auto& nodes = m_context.Nodes();
        switch (nodes[expr].GetKind())
        {
        case NodeKind::BinaryExpr:
        {
            const auto& binary = nodes.Get<BinaryExpression>(expr);
            if (!binary.IsLogical())
            {
                return LetsUndefinedOut(binary.GetLeftExpr()) || LetsUndefinedOut(binary.GetRightExpr());
            }
            if (LetsUndefinedOut(binary.GetLeftExpr()))
            {
                return true;
            }
            const auto& left = OperandSummary(binary.GetLeftExpr());
            if (left.failed || left.strictUndefined || !left.truth)
            {
                return !left.failed && left.strictUndefined;
            }
            // `a or b` is b when a is false: b is not tested
            return *left.truth != binary.IsOr() && LetsUndefinedOut(binary.GetRightExpr());
        }
        case NodeKind::ConcatExpr:
            for (const auto operand : nodes[nodes.Get<ConcatExpression>(expr).GetOperands()])
            {
                if (LetsUndefinedOut(operand))
                {
                    return true;
                }
                const auto& value = OperandSummary(operand);
                if (value.failed || value.strictUndefined)
                {
                    return !value.failed;
                }
            }
            return false;
        case NodeKind::FullExpr:
        {
            const auto& full = nodes.Get<FullExpressionEvaluator>(expr);
            if (!full.GetTester())
            {
                return LetsUndefinedOut(full.GetExpression());
            }
            const auto& conditional = nodes[full.GetTester()];
            if (LetsUndefinedOut(conditional.GetTestExpr()))
            {
                return true;
            }
            const auto& test = OperandSummary(conditional.GetTestExpr());
            if (test.failed || test.strictUndefined || !test.truth)
            {
                return !test.failed && test.strictUndefined;
            }
            return LetsUndefinedOut(*test.truth ? full.GetExpression() : conditional.GetAltValue());
        }
        case NodeKind::UnaryExpr:
            return LetsUndefinedOut(nodes.Get<UnaryExpression>(expr).GetOperand());
        case NodeKind::IsExpr:
            return LetsUndefinedOut(nodes.Get<IsExpression>(expr).GetValue());
        case NodeKind::FilteredExpr:
            return LetsUndefinedOut(nodes.Get<FilteredExpression>(expr).GetOperand());
        case NodeKind::SubscriptExpr:
            return LetsUndefinedOut(nodes.Get<SubscriptExpression>(expr).GetValue());
        default:
            return false;
        }
    }

    // What an operand evaluates to: its candidate's step when it is one, else its own evaluation, memoized
    const FoldSummary& OperandSummary(NodeRef<Expression> expr)
    {
        const auto& nodes = m_context.Nodes();
        const void* key = &nodes[expr];
        if (const auto found = m_candidateOf.find(key); found != m_candidateOf.end())
        {
            return SummaryOf(found->second);
        }
        if (const auto found = m_operands.find(key); found != m_operands.end())
        {
            return found->second;
        }
        const auto value = TryFoldValue([this, &nodes, expr] { return nodes[expr].Evaluate(m_context); });
        return m_operands.emplace(key, Summarize(value, false)).first->second;
    }

    // The candidate's value, evaluated; a step's from the step before it
    const FoldSummary& SummaryOf(std::size_t index)
    {
        Compute(index);
        auto& step = m_steps[index];
        if (!step.summarized)
        {
            step.summary = Summarize(step.value, !m_check.candidates[index].output);
            step.summarized = true;
        }
        return step.summary;
    }

    void Compute(std::size_t index)
    {
        if (m_steps[index].known)
        {
            return;
        }
        // The steps down to one whose value is known, or the chain's first, evaluated whole
        std::vector<std::size_t> pending{ index };
        while (m_steps[pending.back()].previous != NoStep && !m_steps[m_steps[pending.back()].previous].known)
        {
            pending.push_back(m_steps[pending.back()].previous);
        }
        while (!pending.empty())
        {
            const auto step = pending.back();
            pending.pop_back();
            const auto previous = m_steps[step].previous;
            if (previous != NoStep && !m_steps[previous].released)
            {
                m_steps[step].value = NextStep(m_steps[previous].value, step);
            }
            else
            {
                // The first of the chain, or one whose operand's value is gone (only a candidate already done reads
                // no value: a safety net)
                m_steps[step].value = TryFoldValue([this, step] { return m_context.Nodes()[m_check.candidates[step].node].Evaluate(m_context); });
            }
            m_steps[step].known = true;
            ReadDone(step);
        }
    }

    // The step has read the value of the one before it, or never will
    void ReadDone(std::size_t index)
    {
        auto& step = m_steps[index];
        if (step.readDone)
        {
            return;
        }
        step.readDone = true;
        if (step.previous != NoStep)
        {
            --m_steps[step.previous].readers;
            Release(step.previous);
        }
    }

    void Done(std::size_t index)
    {
        m_steps[index].done = true;
        ReadDone(index);
        Release(index);
    }

    // Drops the step's value once nothing reads it: its summary is what is left of it
    void Release(std::size_t index)
    {
        auto& step = m_steps[index];
        if (step.readers != 0 || !step.known || step.released)
        {
            return;
        }
        if (!step.summarized)
        {
            step.summary = Summarize(step.value, !step.done && !m_check.candidates[index].output);
            step.summarized = true;
        }
        step.value = FoldValue();
        step.released = true;
    }

    FoldValue NextStep(const FoldValue& previous, std::size_t index)
    {
        if (previous.failed)
        {
            return previous;
        }
        const auto& nodes = m_context.Nodes();
        const auto node = m_check.candidates[index].node;
        switch (m_steps[index].kind)
        {
        case StepKind::Filter:
        {
            const auto chain = nodes.Get<FilteredExpression>(node).GetFilter();
            return TryFoldValue([this, &nodes, chain, &previous] { return nodes[chain].EvaluateLast(previous.value, m_context); });
        }
        case StepKind::Binary:
            return TryFoldValue([this, &nodes, node, &previous] { return nodes.Get<BinaryExpression>(node).EvaluateWithLeft(previous.value, m_context); });
        case StepKind::Unary:
            return TryFoldValue([&nodes, node, &previous] { return nodes.Get<UnaryExpression>(node).EvaluateWith(previous.value); });
        case StepKind::Subscript:
            return TryFoldValue([this, &nodes, node, &previous] { return nodes.Get<SubscriptExpression>(node).EvaluateWithValue(previous.value, m_context); });
        case StepKind::Test:
            return TryFoldValue([this, &nodes, node, &previous] { return nodes.Get<IsExpression>(node).EvaluateWith(previous.value, m_context); });
        case StepKind::None:
            break;
        }
        return TryFoldValue([this, &nodes, node] { return nodes[node].Evaluate(m_context); });
    }

    // Jinja2's CondExpr.as_const: the test's as_const, whose truth fails for a strict undefined
    // and lets the UndefinedError out; then that of the branch it picks, Impossible without one
    FoldOutcome ConditionalOf(const FullExpressionEvaluator& full)
    {
        CheckStack();
        const auto& nodes = m_context.Nodes();
        const auto& conditional = nodes[full.GetTester()];
        if (!conditional.TestFoldsAt(m_context))
        {
            return FoldOutcome::DoesNotFold;
        }
        const auto test = TryFoldValue([this, &nodes, &conditional] { return nodes[conditional.GetTestExpr()].Evaluate(m_context); });
        // A failing test or branch is Impossible: what Jinja2 lets out of one is the strict
        // undefined's error of an `and`, `or` or `~` in it, a candidate of its own
        if (test.failed)
        {
            return FoldOutcome::Fails;
        }
        if (m_check.strictUndefined && test.value.IsUndefined())
        {
            return FoldOutcome::FailsLoad;
        }
        const bool truth = ConvertToBool(test.value);
        if (!conditional.BranchFoldsAt(m_context, full.GetExpression(), truth))
        {
            return FoldOutcome::DoesNotFold;
        }
        const auto branch = truth ? full.GetExpression() : conditional.GetAltValue();
        if (const auto chained = nodes.As<FullExpressionEvaluator>(branch); chained && nodes[chained].GetTester())
        {
            return ConditionalOf(nodes[chained]);
        }
        const auto value = TryFoldValue([this, &nodes, branch] { return nodes[branch].Evaluate(m_context); });
        if (value.failed)
        {
            return FoldOutcome::Fails;
        }
        return HasSafeRepr(value.value) ? FoldOutcome::Folds : FoldOutcome::DoesNotFold;
    }

    const FoldCheck& m_check;
    RenderContext& m_context;
    std::vector<Step> m_steps;
    // The latest candidate of each node
    std::unordered_map<const void*, std::size_t> m_candidateOf;
    // LetsUndefinedOut's answers, and the summaries of the operands it evaluated that are no candidates
    std::unordered_map<const void*, bool> m_letsOut;
    std::unordered_map<const void*, FoldSummary> m_operands;
};
} // namespace

bool FoldCheckPasses(const FoldCheck& check, RenderContext& context)
{
    // The candidates were recorded as their parse ended, so an expression comes after the ones
    // inside it, which start where it starts or later. Walked outermost first, the candidates
    // inside one that folds need no evaluation of their own: their subscripts are covered, and
    // nested brackets do not evaluate the inner levels again and again
    std::vector<FoldOutcome> outcomes(check.candidates.size(), FoldOutcome::DoesNotFold);
    FoldEvaluator evaluator(check, context);
    unsigned foldedFrom = check.to;
    unsigned outputFoldedFrom = check.to;
    for (std::size_t index = check.candidates.size(); index-- != 0;)
    {
        const auto& candidate = check.candidates[index];
        // Jinja2's optimizer folds every expression of the tree, and a strict undefined that
        // fails a fold fails the compile (UndefinedError) where an Impossible would not: under
        // StrictUndefined each candidate is evaluated, except inside an output that folds,
        // which the compiler folds whole before the optimizer sees its parts (visit_Output,
        // docs/tasks/0484)
        if (candidate.from >= (check.strictUndefined ? outputFoldedFrom : foldedFrom))
        {
            evaluator.Skip(index);
            continue;
        }
        outcomes[index] = evaluator.Of(index);
        if (check.strictUndefined && (outcomes[index] == FoldOutcome::FailsLoad || (outcomes[index] == FoldOutcome::Fails && candidate.failsLoad)))
        {
            return false;
        }
        if (outcomes[index] == FoldOutcome::Folds)
        {
            foldedFrom = candidate.from;
            outputFoldedFrom = candidate.output ? candidate.from : outputFoldedFrom;
        }
    }
    return CoversEverySubscript(check, [&outcomes](std::size_t index) { return outcomes[index] == FoldOutcome::Folds; });
}

} // namespace v2
} // namespace jinja2
