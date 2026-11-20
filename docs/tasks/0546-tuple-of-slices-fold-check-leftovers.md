---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/963
priority: low
area: parity
depends: [0536]
touches: [src/expression_parser.cpp#FoldCheckPasses]
shares: [test/limits_test.cpp]
---
# Tuple-of-slices fold check: argument steps, subscript levels, kept values and the per-render constant

**Problem.** Left by 0536, which links filter, binary and unary candidates as steps of a chain at Load:

- a candidate inside a filter's or call's argument, or a subscript level of nested brackets
  (`[1][:, :][0][0]...`), is no step of the candidate around it: the outer one evaluates it again, so
  nesting N levels evaluates about N^2/2 nodes (bounded by maxExpressionDepth and the render limits);
  comparison chains and tests (`x is y`) are not linked either;
- a kept step value lives until the check ends: a chain of large values (`|center(3000000)` over
  hundreds of steps) holds them all, bounded by maxExpressionOperators times the largest value;
- per render: Jinja2 replaces the folded subscript by its constant, while the sealed tree keeps a
  TupleSliceExpression that evaluates its operand and parts and builds the undefined's name each time
  it runs. Caching it needs per-template storage for the constants the Load computed, since sealed
  nodes are shared and immutable (Codex P2 on #865).

- with StrictUndefined `failsLoad` of an `and`, `or` or `~` candidate takes any failure of an operand for
  the UndefinedError Jinja2 lets out, where a unary operator's or a filter's failure is Impossible: Jinja2
  renders `b` for `{% if false and (none or (not (1, 2)[:, :])) %}a{% endif %}b`,
  `{% if false and (-([1, 2]) or ([1][:, :] + 1)) %}a{% endif %}b` and
  `{% if false and ((false) ~ ([1][:, :]|float)) %}a{% endif %}b`, which fail the Load here (on master too).
  `FoldCandidate::leadingOperands` now finds the real UndefinedError cases, so `failsLoad` could narrow to them;
- a strict undefined from another source than a tuple-of-slices subscript fails Jinja2's compile as a
  conditional's test or in a `~` (`{{ 1 if []|max else 2 }}`, `{{ 1.5 ~ ([]|max) }}`) and fails the render
  here: the fold check runs only for templates with such subscripts (verifier on 0536).

**Proposal.** Link SubscriptExpression and IsExpression candidates on their value operand as 0536 links
operators, drop a step's value once its last later step has read it, and measure whether the per-render
constant is worth a per-template table.

**Done when.** A strict nesting of N subscript levels around a tuple-of-slices subscript evaluates O(N) nodes
at Load (a LimitsEnvTest step-limit test), and the kept values are freed as the chain passes them.

**Result (parity batch 11 B).** FoldEvaluator links a subscript (`SubscriptExpression`, not `loop.x`), a test
(`IsExpression`) and a chain's first filter whose operand is a candidate as a step after it, as 0536 links
operators: `SubscriptExpression::EvaluateWithValue`, `IsExpression::EvaluateWith` and `EvaluateLast` on the kept
value, and their fold from the step before (`IndicesFoldAt`, `TestFolds`). 150 levels of `((x)|string)[0]` or
`(x is true)` around a tuple-of-slices subscript load in under 5000 steps
(`LimitsEnvTest.StrictSubscriptAndTestStepsFoldInLinearSteps`; master needs N^2/2). Each step counts the steps
that read it; once they have and its own candidate is done (or skipped inside a fold), its value is dropped and
a summary (failed, undefined, strict undefined, truth, safe repr) answers for it, so a chain holds about two
values (a list or dict step stays until its own candidate is done, so that its repr is walked only for its own
outcome, never for a step a fold around it skips; Codex on #963) (`LimitsEnvTest.StrictFoldStepsDropTheValuesTheyPassed`: 100 `|center(200000+i)` steps in 4 MiB of
maxRenderMemory). An `and`, `or` or `~` candidate whose evaluation fails no longer fails the Load as such: it
does when `FoldEvaluator::LetsUndefinedOut` finds the UndefinedError Jinja2 lets out, bool() or str() of a
strict undefined operand of an `and`, `or`, `~` or conditional on the path its as_const evaluates (through
unary operators, subscripts, tests and filter operands too, as the optimizer folds them first); any other
failure is Impossible (corpus `subscripts.tuple_of_slices_strict_impossible_operands` and the `_nested_*` cases).

Per-render constant: measured with the Python module (RelWithDebInfo), 100 outputs of `[1][:, :] is undefined`
render in 45 µs against 10 µs for `[1] is undefined` and 1.6 µs for `true`: about 0.35 µs per tuple-of-slices
evaluation. Such subscripts exist only because Jinja2's fold makes invalid Python compile, so no real template
renders many of them; a per-template constant table is not worth its storage and the Load work. Not done.

Left: candidates inside a filter's or call's argument and items of a list, tuple or dict literal are no steps
(`1|default(...)` and `[[...]]` nesting is still N^2/2 at Load, bounded by maxExpressionDepth): 0554. A strict
undefined from another source than a tuple-of-slices subscript fails Jinja2's compile in an `and`, `or`, `~` or
conditional, also where it never renders; the engine checks folds only around such subscripts: 0555.
