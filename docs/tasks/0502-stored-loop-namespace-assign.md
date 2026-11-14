---
status: open
priority: low
area: parity
depends: [0482]
touches: [src/statements.cpp, src/render_context.h]
shares: [test/template_module_test.cpp]
---
# A stored recursive loop that sets a namespace attribute fails after its include ended

**Problem.** Found while implementing 0482. A recursive loop of an included template, stored in a
module's namespace through `loop` and called by a later call after the include has ended, fails
with `cannot assign attribute on non-namespace object` when its body runs
`{% if loop.first %}{% set ns.f = loop %}{% endif %}` (the namespace name resolves to something
else once the include's frame is gone). Jinja2 3.1.6 renders the body normally. 0482's
recursive-loop row avoids it with `loop.depth == 1`.

**Proposal.** Resolve the loop body's names at the loop's `ClosureSite` (as a macro's body does,
0038 lexical scoping) when the recursive callable runs after its frame ended.

**Done when** a `TemplateModuleChangingSourceTest` row with the `loop.first` variant prints what
Jinja2 3.1.6 prints, ASan clean.
