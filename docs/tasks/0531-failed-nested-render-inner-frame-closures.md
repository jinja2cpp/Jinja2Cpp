---
status: open
priority: low
area: parity
depends: [0525]
touches: [src/statements.cpp]
shares: [test/borrowed_cells_test.cpp]
---
# A closure defined in a frame that a failing nested render went out of reads its names as ended

**Problem.** Found while fixing 0525. A render nested in another one that shares its globals now
closes over its top level when it fails (`TemplateImpl::RenderForOuter`), and over the scopes the
failure left. But a frame whose names are in slots, not scopes, is let go of on the way out before
that: a macro call's params (`UnitCall`), a loop's variables (`SlotsGuard`), a `with` body's names.
A closure defined there and stored in a shared global, called by the outer render after the host
swallowed the nested error, prints `` where Python Jinja2 3.1.6 prints the names the frame held:

| Nested template (rendered by `inner()`, error swallowed), outer `{{ inner() }}{{ g[-1]() }}` | Python |
|---|---|
| `{% macro o(n) %}{% macro i() %}{{ n }}{% endmacro %}{% set _ = g.append(i) %}{{ 1 // 0 }}{% endmacro %}{{ o('on') }}` | `on` |
| `{% for v in ['lv'] %}{% macro i() %}{{ v }}{% endmacro %}{% set _ = g.append(i) %}{{ 1 // 0 }}{% endfor %}` | `lv` |
| `{% with w = 'wv' %}{% macro i() %}{{ w }}{% endmacro %}{% set _ = g.append(i) %}{{ 1 // 0 }}{% endwith %}` | `wv` |

Memory-safe. Only a render nested under a keeper is affected: elsewhere a failure ends the render
and nothing calls the closure.

**Proposal.** Where those guards let go of a frame on an exception, close over it first as a stop
does (`FrameEnd::Stopped`, `EndFailedMacroBody`), when the context ends before the render
(`RenderContext::EndsBeforeRender`) and has captures; out of line, so that the success path pays
nothing.

**Done when.** The three rows above are `BorrowedCellsSingle.NestedRenderThatFailsKeepsItsClosures`
rows with Python's output, ASan clean.
