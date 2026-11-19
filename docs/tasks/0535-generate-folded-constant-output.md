---
status: open
priority: low
area: parity
depends: [0456, 0493]
touches: [src/renderer.h, src/expression_evaluator.h, src/expression_evaluator.cpp]
shares: [python/tests/test_generate.py, test/output_test.cpp]
---
# generate() items of constant output that is not a literal

**Problem.** Found by the 0493 verifier. Jinja2 3.1.6 folds a `{{ }}` whose expression it can evaluate when it
compiles the template (`as_const`: `1 + 1`, `'a' | upper`, `'a' ~ 'b'`, `[1, 2]`, `-1`, `not true`,
`x if 0 else 'e'`) into the text around it, and yields that run as one `str` item, under autoescape too.
`ExpressionRenderer::IsLiteralAt` folds only literals (0456), so such an output is an item of its own here,
and with 0493's `RenderOptions::pieceIsMarkup` it is flagged Markup under autoescape, where Jinja2's item is
`str`. Checked with the pinned oracle: `Environment(autoescape=True).from_string('a{{ 1 + 1 }}b').generate()`
gives `['a2b']` of type `str`; here `['a', Markup('2'), 'b']`.

**Proposal.** Let a render that writes each piece ask whether the expression folds (`Expression::IsFoldable`
on the sealed tree, an `ArenaView` overload, or a fold flag the parser records for `{{ }}` only when a
render asks, keeping 0456's Load cost note), and treat it as `PieceKind::Folded` (which also marks it `str`).
Folds by choice (`FoldByChoice`) evaluate the test as `FinalizedExpressionRenderer::FoldsAt` does.

**Done when.** The cases above yield Jinja2's items and types through `generate()` with and without
autoescape (python/tests/test_generate.py `test_items_are_markup_where_jinja2s_are`), and
`OutputTest.SinkLearnsWhichPiecesAreMarkup` has the rows; Load stays within the perf gate.

**Result (parity batch 10 B, with 0505).** A folded constant is `PieceKind::Folded`, so it joins the text around
it as one `str` item under autoescape too: `a{{ 1 + 1 }}b` gives `['a2b']`
(`OutputTest.SinkLearnsWhichPiecesAreMarkup`, `test_items_are_markup_where_jinja2s_are`).
