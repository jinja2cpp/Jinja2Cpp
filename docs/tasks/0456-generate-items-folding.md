---
status: in-progress
priority: low
area: parity
depends: [0436]
touches: [src/renderer.h, src/statements.cpp, src/template_parser.cpp]
shares: [python/tests/test_generate.py]
---
# generate() items where Jinja2 folds output into one

**Problem.** With `RenderOptions::writeEachPiece` (0436) the module's `generate()` yields Jinja2's
items for text, `{{ }}`, loops, conditions, macro and call blocks, but these cases differ from
Jinja2 3.1.6 (checked with the pinned oracle):

- Constant output is folded by Jinja2's compiler into the text around it: `a{{ 1 }}b` is one item
  `"a1b"` in Jinja2, three here (`"a"`, `"1"`, `"b"`).
- `{{ super() }}` is one item in Jinja2 (the parent block's output joined), several here.
- A recursive loop (`{% for i in seq recursive %}...{{ loop(x) }}...{% endfor %}`) is buffered by
  Jinja2 and yielded as one item; here each piece is an item.
- `{{ self.b() }}` is one item in Jinja2 (`A5B`), three here.
- Text split by a comment (`a{# c #}b`) or by `{% raw %}` is one item in Jinja2, two here.
- Constants of other types fold too: `{{ none }}{{ true }}{{ 1.5 }}` is one item in Jinja2, three
  here; under autoescape `{{ '<' }}a{{ '<' }}` is one item, three here.
- `{{ x }}` of an empty value (an undefined name) is an item `''` in Jinja2; the sink never gets an
  empty write, so no item here. The same for an empty macro call (`{{ m() }}`), an empty filter
  block, `{{ '' }}` and `{{ x if not x }}`.
- A macro or call block whose output is longer than the stream's 512-character buffer is one item
  in Jinja2 and several here (the buffer flushes when full; found by the 0436 verifier round 2):
  a 900-character macro gives `[384, 384, 132]`.

**Proposal.** Hold the piece flush (`OutStream::PieceHold`) in `super()` and in a recursive loop's
body, and mark constant-folded runs when the parser merges them (or merge adjacent constant
nodes at parse time, which also helps the render). Let a held piece grow past the buffer (or hand the sink the held run in one
call). Add the cases to
`OutputTest.SinkGetsEachPieceWhenAsked` and python/tests/test_generate.py.

**Done when.** These cases yield Jinja2's items through `generate()`.

**Result (parity batch 7).** A body that writes each piece (`ComposedRenderer::RenderNodes<true>`) asks
each node what it is as a piece (`PieceOf`): template text outside volatile code and a `{{ }}` whose
expression Jinja2 folds (`Expression::PrepareFold`, kept in the ExpressionRenderer at Load) run together
into one piece; any other output node (a `{{ }}` evaluated at run time, a filter or call block) ends
its piece with `OutStream::FlushPiece`, which writes an empty piece too. `PieceHold` now covers
`super()`, `self.b()` and a recursive loop, and holds what overflows the buffer until the piece ends
(`OutStream::m_held`, counted as written for the output limit), so a long macro is one Write. All the
cases above match Jinja2 3.1.6 (OutputTest.SinkGetsThePiecesJinja2Folds, test_generate.py). Left: the
items' Markup type (0493).
