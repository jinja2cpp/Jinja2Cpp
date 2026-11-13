---
status: open
priority: low
area: parity
depends: [0456]
touches: [src/renderer.h, src/expression_evaluator.h, src/expression_evaluator.cpp]
shares: [test/output_test.cpp, python/tests/test_generate.py, docs/python.md]
---
# generate() items: Jinja2 folds a constant operator, subscript or filter into the text

**Problem.** The remainder of 0456. Jinja2's compiler folds any `{{ }}` its optimizer evaluates at
compile time (`as_const`) into the template text around it, so `generate()` yields one item for the
run. 0456 folds only a literal, because the walk that decides foldability (`PrepareFold`) cost
Load/expressions +3.2% when every `{{ }}` ran it at Load. A render that writes each piece
(`RenderOptions::writeEachPiece`, the module's `generate()`) therefore splits these into items of their
own:

```
a{{ 1 + 2 }}b{{ 'q'|upper }}c   Jinja2: ['a3bQc']        here: ['a', '3', 'b', 'Q', 'c']
{{ [1][0] }}x                   Jinja2: ['1x']           here: ['1', 'x']
a{{ 1 if true else y }}b        Jinja2: ['a1b']          here: ['a', '1', 'b']
```

The joined output is the same; only the item boundaries differ. `OutputTest.SinkGetsThePiecesJinja2Folds`
pins the current split (row `a{{ 1 + 2 }}b{{ 'q'|upper }}c`, "Only a literal folds here"), and
docs/python.md lists it.

**Proposal.** Decide the fold when a render that writes each piece asks, as 0456's final form already
does for literals (`ExpressionRenderer::IsLiteralAt`, `Expression::ConstantAt` on the sealed tree):
run the foldability walk then, on the sealed tree (`ArenaView`), and cache its answer per renderer so a
template rendered by `generate()` many times walks once. Keep the usual render and Load untouched: no
new per-node field unless it fits existing padding, and the walk only on the piece path. The
conditional fold (`FoldByChoice`, `IsFoldableAt`) applies as in `FinalizedExpressionRenderer`.

**Done when.** The three templates above yield Jinja2's items through the module and in
`SinkGetsThePiecesJinja2Folds` (narrow and wide, with the row's expectation updated), the docs/python.md
bullet is dropped, and `bench/count.py` shows Load and the non-piece Render/RenderSink rows within
+0.5% of master.
