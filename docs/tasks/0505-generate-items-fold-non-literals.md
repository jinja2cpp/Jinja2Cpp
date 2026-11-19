---
status: open
priority: low
area: parity
depends: [0456]
touches: [src/renderer.h, src/expression_evaluator.h, src/expression_evaluator.cpp, src/expression_parser.cpp, src/expression_parser.h, src/node_arena.h, src/node_arena.cpp, src/template_parser.cpp]
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

**Result (parity batch 10 B).** A `{{ }}` Jinja2 folds is a node kind of its own (`FoldedExpressionRenderer`,
`FoldedByChoiceRenderer`, which evaluates the test as `FinalizedExpressionRenderer::FoldsAt` does), so the usual
`{{ }}` keeps its size and a render that writes each piece reads the kind (`PieceOf`). The parser walks an
output's expression (PrepareFold) only when it read no name, or holds a conditional whose test or an
`and`/`or` whose left operand read none (`ExpressionParser::m_readsName`, `m_foldChoice`): an expression that
reads a name folds only by the side such a test picks (FULL contract checks assert it on every `{{ }}`). Whether it folds then takes evaluating it, as Jinja2's as_const does: a value that fails, or a strict undefined str() refuses, is a piece of its own (`OutputTest.SinkGetsThePiecesBeforeAFailingConstant`). The
three templates yield Jinja2's items (`OutputTest.SinkGetsThePiecesJinja2Folds`, test_generate.py); the
docs/python.md bullet is gone. `bench/count.py` against master: Load +0.20% to +0.45% (chat_qwen, the most).
