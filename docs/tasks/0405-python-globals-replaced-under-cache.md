---
status: open
priority: low
area: parity
depends: [0353]
touches: [python/src/environment.cpp#MakeTemplate, python/src/environment.cpp#GetTemplate]
shares: [python/tests/test_render.py, docs/python.md]
---
# Templates cached before env.globals is replaced see the new dict

**Problem.** In Jinja2 a cached template, and a module imported from it, keeps the globals dict
it was made with: after `env.globals = {...}`, `get_template` of a cached name still sees the
old dict. The module makes a new `Template` on each `get_template` with the dict of that
moment, and an imported template sees the importer's (found by the 0353 verifier, in
docs/python.md "Differences").

**Proposal.** Store the globals dict with the cached engine template (the cache entry, not the
Python wrapper), and pass the imported template its own stored dict instead of the importer's.
Check against the design principle (host data immutable, renders independent): the dict is
captured, not shared mutably, so it fits.

**Acceptance.** pytest cases for replace-then-get_template and for import after replace match
Jinja2; the docs/python.md line is removed.

**Decision (Ruslan, 2026-11-20, batch 10 D): a deliberate divergence.** The engine gives a render one
global scope, which its includes, imports and extends share. Matching Jinja2 would need a scope per
nested template (`statements.cpp` import and include, `RenderContext`'s global scope, `template_impl.h`
loaded templates): the globals converted again at each nested load of every render, and a new
lifetime class for the converted values that escaping macros and modules hold. It would only change
a replaced (not edited) `env.globals` or a per-template `globals=` seen through an import or a
`without context` include. docs/python.md "Differences" records it; the 0410 design (PR #953) left
room for an additive `TemplateEnv` hook on the same `LoadTemplateAt` seam if it is ever wanted.

