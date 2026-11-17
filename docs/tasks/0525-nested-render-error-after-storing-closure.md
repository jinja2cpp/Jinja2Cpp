---
status: in-progress
priority: low
area: parity
depends: [0491]
touches: [src/template_impl.h]
shares: [test/borrowed_cells_test.cpp]
---
# A closure stored by a nested render that then fails reads its names as ended

**Problem.** Found by the verifier of batch 8 C (0491). A render nested in another one closes over its
top level and its modules when it ends (`RendererCallback::CaptureClosuresForOuter`), but only when it
succeeds. When it fails after storing a macro in a shared global
(`{% set x = 'cl' %}{% macro m() %}{{ x }}{% endmacro %}{% set _ = g.append(m) %}{{ 1 // 0 }}`), the outer
render's `g[-1]()` prints `` where Python Jinja2 3.1.6 prints `cl` (the host swallows the nested error).
Memory-safe. A stored `self` or `super()` already matches.

**Proposal.** Close over the contexts on the failure path too, before the render's frames go (as
`EndFailedMacroBody` does for a stopped macro), out of line.

**Done when.** A `BorrowedCellsSingle` row with the template above prints Python's output, ASan clean.

**Resolution.** A render under a keeper now runs in `TemplateImpl::RenderForOuter`, which closes over
its contexts whether the render ends or fails. On a failure it first ends the scopes the failure left
behind, innermost first, keeping their names (`FrameEnd::Stopped`, as Jinja2 never reaches their
ends), then closes over the top level and the modules (`CaptureClosuresForOuter`), then rethrows
outside the handler. The render that nests in nothing pays nothing new. Rows in
`BorrowedCellsSingle.NestedRenderThatFailsKeepsItsClosures` (the reproducer, a failure in a loop body
after the top level set the name again, a module's macro, a failure in a `with`), with Python's output.
A closure defined in a frame whose names are slots (a macro call, a loop, a `with`) still reads them
as ended once the failure went out of it: filed as 0531.
