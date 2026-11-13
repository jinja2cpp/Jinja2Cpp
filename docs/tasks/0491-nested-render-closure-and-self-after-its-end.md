---
status: open
priority: low
area: parity
depends: [0481]
touches: [src/statements.cpp, src/render_context.h]
shares: [test/borrowed_cells_test.cpp]
---
# A nested render's closures and `self` stored in a shared global lose their frames

**Problem.** Found while fixing 0481. A render nested in another one shares its globals (0324,
0463), and the outer render keeps the nested template alive (0481), so a macro the nested render
stores in a global runs safely there afterwards. What it closed over is gone, though: its home
context ended with the nested render, so the call runs with every scope hidden (`ClosureSite::Ended`).

| nested render stores | outer render calls | Python 3.1.6 | Jinja2C++ |
|---|---|---|---|
| `{% set x = 'cl' %}{% macro m() %}{{ x }}{% endmacro %}` → `g.append(m)` | `g[-1]()` | `cl` | `` |
| `{% macro o(n) %}{% macro i(k) %}{{ n }}{{ k }}{% endmacro %}{% set _ = g.append(i) %}{% endmacro %}{{ o('cap') }}` | `g[-1]('k')` | `capk` | `k` |
| `{% block b %}blk{% endblock %}` → `g.append(self)` | `g[-1].b()` | `blk` | error: a template handle used outside the render that made it |
| `{% macro m2() %}{{ caller(5) }}{% endmacro %}{% macro m() %}{% call(q) m2() %}c{{ q }}{% endcall %}{% endmacro %}` → `g.append(m)` | `g[-1]()` | `c5` per call | error: 'm2' is undefined |
| a module macro `f` whose module did `{% from 'modB' import m2 %}`, `f` calls `m2()` → `g.append(mod.f)` | `g[-1]()` | `f:deep` | error: 'm2' is undefined |
| a block's `super` → `g.append(super)` | `g[-1]()` | the parent block | error: a template handle used outside the render that made it |

Pinned by `BorrowedCellsSingle.NestedRenderValueOutlivesItsTemplate` (last two rows) and
`BorrowedCellsSingle.NestedRenderSelfOutsideItsRender`. Memory-safe: a divergence only.

**Proposal.** When a render that shares its outer render's snapshot ends, its top level is copied
for the closures that escaped into the outer render's cells, as 0326 copies a macro call's frame
(`RenderContext::CaptureClosures`, `ClosureCapture`), owned by the outer render
(`CycleCollector::KeepUntilEnd` of that render's collector). `self` needs its handles made valid
for the outer render (`TemplateSlots`) or a keep-alive of the nested render's slots.

A macro calling a sibling macro of its template is the common case (found by the 0481 verifier).

**Done when.** The rows print Python's output, ASan clean.
