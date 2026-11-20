---
status: open
priority: low
area: parity
depends: [0546]
touches: [src/expression_parser.cpp#FoldCheckPasses]
shares: [test/limits_test.cpp, src/expression_evaluator.h, src/expression_evaluator.cpp]
---
# Tuple-of-slices fold check: candidates inside arguments and literal items

**Problem.** Left by 0546, which links subscripts, tests and a chain's first filter as steps of the candidate
they apply to. A candidate inside a filter's or call's argument (`1|default(x)`) or an item of a list, tuple
or dict literal (`[x]`) is no step of the candidate around it: under StrictUndefined every candidate is
evaluated, and the outer one evaluates the inner one again, so N levels of `1|default(...)` or `[...]` around
a tuple-of-slices subscript evaluate about N^2/2 nodes at Load (measured on 0546: 50 levels of
`1|default(...)` take 5512 render steps, 25 take 1512; 50 levels of `[...]` 2758). It is bounded by
maxExpressionDepth (70 brackets by default) and the render limits, so it costs time, not safety.

**Proposal.** Give the fold check a way to evaluate a node with one operand's value given: an argument's in
the filter's call parameters, an item's in the literal, without a per-node branch on the render path (a
FoldEvaluator-only evaluation, as `EvaluateWithLeft` and `EvaluateLast` are).

**Done when.** A strict nesting of N filter arguments or list literals around a tuple-of-slices subscript
evaluates O(N) nodes at Load (a LimitsEnvTest step-limit test with maxExpressionDepth raised).
