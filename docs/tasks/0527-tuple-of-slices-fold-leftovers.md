---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/935
priority: low
area: parity
depends: [0497]
touches: [src/expression_parser.cpp#FoldCheckPasses]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt]
---
# Tuple-of-slices folds: StrictUndefined leftovers and the cost of the Load's checks

**Problem.** Left by 0497 (and 0484 before it):

- with StrictUndefined, FoldCandidate::failsLoad is decided by the candidate's node kind, so a
  conditional does not pass on the Impossible of a filter in a branch:
  `{% if false and ([1][:, :]|length if 1 else x) %}a{% else %}b{% endif %}` renders `b` in Jinja2
  and fails the engine's Load (`subscripts.tuple_of_slices_strict_conditional_branch`); and
  `{% if false and (1 if [1][:, :] else x) %}` fails Jinja2's compile (the test's truthiness of a
  strict undefined) where the engine renders `b` (`subscripts.tuple_of_slices_strict_conditional_test`);
- with StrictUndefined `{{ [1][:, :]|batch(2) }}`, `|slice(2)`, `|unique` and `|default([])|items` fold in Jinja2, whose
  filters return generators, and fail in the engine, which evaluates them eagerly (no case: the
  output is a generator's repr with its address);
- cost: under StrictUndefined every candidate is evaluated, and every operator node and, since
  0497, every step of a filter chain around a tuple-of-slices subscript is one, so a chain of N
  operators or filters evaluates about N^2/2 nodes at Load (N is bounded by
  Settings::Limits::maxExpressionOperators, 491); nested brackets evaluate each level's candidate; a
  host filter in a candidate is called once per candidate it is in, where Jinja2 calls it once.
  Each evaluation is bounded by the render limits;
- per render: Jinja2 replaces the folded subscript by its constant, while the sealed tree keeps a
  TupleSliceExpression that evaluates its operand and parts and builds the undefined's name each
  time it runs. Caching it needs per-template storage for the constants the Load's fold check
  computed, since sealed nodes are shared and immutable (Codex P2 on #865).

**Proposal.** Decide failsLoad of a conditional from its branches (Jinja2's CondExpr.as_const
evaluates the test, then the chosen branch's as_const); keep the value of a step's evaluation for
the next step's candidate instead of evaluating the chain again, within the Load perf gate.

**Done when.** The two `tuple_of_slices_strict_conditional_*` divergence lines are gone, and a
strict chain of N filters around a tuple-of-slices subscript evaluates O(N) filters at Load.

**Resolution (batch 9 B).** A non-output conditional candidate is evaluated as Jinja2's
`CondExpr.as_const` runs (FoldEvaluator::ConditionalOf in src/expression_parser.cpp): a test or
branch that does not fold is Impossible; a failing test or branch is Impossible for a kind whose
as_const catches it (FoldCatchesFailure) and fails the Load otherwise; a strict undefined test
fails the Load (its truth raises); without `else` a false test is Impossible; an `else` chain is
followed. Filter-chain steps link to the step before them (same operand, parent chain), and a
step's value is its last filter on the kept value of that step (ExpressionFilter::EvaluateLast),
so a strict chain of N filters runs N filters at Load (`LimitsEnvTest.StrictFilterChainFoldsInLinearSteps`,
400 filters under maxRenderSteps 20000, fails on master). The two divergence lines are gone; new
corpus cases `tuple_of_slices_strict_conditional_*` and `tuple_of_slices_strict_filter_chain`.
Generator folds, operator chains, nested brackets and the per-render constant moved to 0536.
