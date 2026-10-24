---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/646
priority: low
area: parity
touches: [src/expression_evaluator.cpp#IfExpression, src/expression_evaluator.cpp#BinaryExpression]
---
# Fold a value chosen by a foldable test that is not a literal

Found by the 0288 verifier. `Expression::IsFoldable` decides whether Jinja2 folds a `{{ }}`,
which sets the escaping order with `Settings::finalize` (docs/tasks/0288) and the escaping
setting in volatile code (docs/tasks/0216). `IfExpression::IsFoldable` takes a branch only
when the test is a literal (`GetConstant`), and otherwise needs both branches foldable;
`BinaryExpression::IsFoldable` needs both operands of `and`/`or`. Jinja2's `CondExpr.as_const`
and `And`/`Or.as_const` evaluate a foldable test and fold the chosen side alone. So C++ treats
as run-time what Python folds, and escapes after finalize instead of before (autoescape on,
`brackets`, `x='<'`):

- `{{ '<'|safe if 1 > 0 else x }}`, `{{ ('<'|safe) or x }}`: Python `[<]`, C++ `[&lt;]`
  (case `autoescape.env_on_finalize_folded_choice`).
- `{{ '<'|e if ('<'|e) is escaped }}`: Python `[&lt;]`, C++ `[&amp;lt;]`.
- With `none_to_empty`: `{{ none if 1 > 0 }}`, `{{ none if 1 > 0 else x }}`: Python `None`,
  C++ empty (Python folds escape(None) = Markup('None')).

The gap only ever escapes more, never less. The conditional without `else` is also noted in
0051.

**Proposal.** At parse time, evaluate a foldable test (or left operand of `and`/`or`) that is
not a literal, the way Jinja2's `as_const` does, and decide foldability by the chosen side.
Errors while evaluating mean "not foldable", as in Jinja2.

**Done when.** `autoescape.env_on_finalize_folded_choice` matches and its line is gone from
`test/parity/divergences/autoescape.txt`.

**Done.** `Expression::PrepareFold` (one walk at Load) says whether a value folds always or
only by the side a foldable test or `and`/`or` operand picks, and records it per operand;
`IsFoldableAt` evaluates those tests at render, where they have the value Jinja2 folds.
`FinalizedExpressionRenderer` and the volatile `StaticOutputRenderer` use it. Cases:
`autoescape.env_on_finalize_folded_choice*`, `autoescape.volatile_folded_choice`. A choice
inside a `~` chain or another composite is still run-time: task 0307.
