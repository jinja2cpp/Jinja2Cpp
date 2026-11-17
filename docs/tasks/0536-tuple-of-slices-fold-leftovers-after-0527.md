---
status: open
priority: low
area: parity
depends: [0527]
touches: [src/expression_parser.cpp#FoldCheckPasses]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt]
---
# Tuple-of-slices folds: generator filters, operator chains and the per-render value

**Problem.** Left by 0527, which made a conditional's fold follow Jinja2's `CondExpr.as_const`
and evaluates a strict filter chain's steps one filter each:

- with StrictUndefined `{{ [1][:, :]|batch(2) }}`, `|slice(2)`, `|unique` and
  `|default([])|items` fold in Jinja2, whose filters return generators, and fail in the engine,
  which evaluates them eagerly (no corpus case: the output is a generator's repr with its
  address);
- under StrictUndefined every operator node around a tuple-of-slices subscript is a candidate
  too, and each is evaluated whole, so a chain of N operators (`[1][:, :] is undefined or a or b
  ...`) still evaluates about N^2/2 nodes at Load; nested brackets evaluate each level's
  candidate. Each evaluation is bounded by the render limits (N by
  Settings::Limits::maxExpressionOperators);
- a filter chain's steps share values only when each step's chain links to the one before it;
  a step inside a filter's argument is evaluated again by the outer step;
- with StrictUndefined a filter that reports a value error through ThrowRuntimeError (an
  ErrorInfo, not a TemplateRuntimeError) fails the Load where Jinja2's fold gives Impossible:
  `{% if false and (1 if [1][:, :]|abs else 2) %}a{% endif %}b` renders `b` in Jinja2 (pre-0527
  too; FoldEvaluator's TryFoldValue catches only TemplateRuntimeError, UndefinedError and
  IntegerOutOfRangeError);
- a conditional whose branches do not both fold is no fold candidate, so its test's truth is not
  checked: `{% if false and (1 if 1 else x if [1][:, :] else x) %}` and
  `{% if false and (0|list if 1 else [1][:, :] ~ x) %}` fail Jinja2's compile (StrictUndefined)
  and load in the engine, as `(x if [1][:, :] else x)` already did;
- a kept step value lives until the check ends: a chain of large values (`|center(3000000)` and
  hundreds of steps) holds them all, bounded by maxExpressionOperators times the largest result;
- per render: Jinja2 replaces the folded subscript by its constant, while the sealed tree keeps a
  TupleSliceExpression that evaluates its operand and parts and builds the undefined's name each
  time it runs. Caching it needs per-template storage for the constants the Load's fold check
  computed, since sealed nodes are shared and immutable (Codex P2 on #865).

**Proposal.** Keep each candidate's value for the candidates around it (an operator's operand is
a candidate recorded before it), as 0527 does for filter steps, without a per-node branch on the
render path; decide whether generator results need a lazy filter path at Load or a deliberate
divergence.

**Done when.** A strict chain of N operators around a tuple-of-slices subscript evaluates O(N)
nodes at Load (a step-limit test as `LimitsEnvTest.StrictFilterChainFoldsInLinearSteps`), and
the generator filters either fold as in Jinja2 or are recorded as a divergence.
