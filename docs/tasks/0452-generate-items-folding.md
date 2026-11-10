---
status: open
priority: low
area: parity
depends: [0436]
touches: [src/renderer.h, src/statements.cpp, src/template_parser.cpp]
shares: [python/tests/test_generate.py]
---
# generate() items where Jinja2 folds output into one

**Problem.** With `RenderOptions::writeEachPiece` (0436) the module's `generate()` yields Jinja2's
items for text, `{{ }}`, loops, conditions, macro and call blocks, but three cases split finer than
Jinja2 3.1.6 (checked with the pinned oracle):

- Constant output is folded by Jinja2's compiler into the text around it: `a{{ 1 }}b` is one item
  `"a1b"` in Jinja2, three here (`"a"`, `"1"`, `"b"`).
- `{{ super() }}` is one item in Jinja2 (the parent block's output joined), several here.
- A recursive loop (`{% for i in seq recursive %}...{{ loop(x) }}...{% endfor %}`) is buffered by
  Jinja2 and yielded as one item; here each piece is an item.

**Proposal.** Hold the piece flush (`OutStream::PieceHold`) in `super()` and in a recursive loop's
body, and mark constant-folded runs when the parser merges them (or merge adjacent constant
nodes at parse time, which also helps the render). Add the three cases to
`OutputTest.SinkGetsEachPieceWhenAsked` and python/tests/test_generate.py.

**Done when.** The three cases yield Jinja2's items through `generate()`.
