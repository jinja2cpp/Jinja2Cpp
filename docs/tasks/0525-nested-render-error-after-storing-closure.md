---
status: open
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
