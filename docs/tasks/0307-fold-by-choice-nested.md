---
status: open
priority: low
area: parity
depends: [0291]
touches: [src/expression_evaluator.cpp#ConcatExpression, src/expression_parser.cpp#ParseStringConcat]
---
# Fold a choice that sits inside another foldable expression

Found while fixing 0291. Jinja2 folds `value if test else alt` and `and`/`or` by the side a
foldable test picks, at any depth: `('<' if 1 > 0 else x) ~ ''` folds, because `Concat.as_const`
folds each operand. 0291 decides that at render time (`Expression::PrepareFold` and
`IsFoldableAt`) only for the conditional, `and`/`or` and the other binary operators, nested in
each other. A `~` chain, a unary operator, a subscript, a slice, a comparison, an `is` test, a
tuple, list or dict literal or a filter whose operand folds only by such a choice is treated as
run-time, so it is escaped by the runtime setting in volatile code and after finalize. The gap
only ever escapes more. `~` is the hard one: the parser already decides at Load whether a
chain is folded (`ConcatExpression::SetFolded`, which drops Markup with `str()`), and a fold by
choice makes that a render-time decision too.

Case: `autoescape.volatile_folded_choice_nested` (Python `<&lt;-1`, Jinja2C++ `&lt;&lt;-1`).

**Proposal.** Give the composite nodes `PrepareFoldByChoice`/`IsFoldableAt` overrides that
record and check their operands the way `BinaryExpression` does, and make the concat chain's
Markup drop follow `IsFoldableAt` when its operands fold by choice.

**Done when.** No line of `test/parity/divergences/` names task 0307, and `ctest -R parity` passes.
