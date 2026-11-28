---
status: open
priority: low
area: parity
depends: [0598]
touches: [src/statements.cpp#ClosesOverOnFailure, src/statements.cpp#ForStatement]
shares: [test/template_module_test.cpp]
---
# A failure out of a loop or `with` in a macro body drops the names its stored closure reads

**Problem.** Found by the verifier of batch 15 A (0598), the same on master b3313a2. 0598 makes a failure out of a
module's top-level `with`, `filter`, loop or block close over the frame first, as a stop does (ClosesOverOnFailure
asks RenderContext::InEndingModule). The same failure inside a macro body still drops the frame's names: a
`Template::MakeModule` macro `keep()` that runs `{% for a in [1] %}{% macro k() %}{{ a }}{% endmacro %}{% set ns.f = k %}{{ 1 // 0 }}{% endfor %}`
and fails leaves `ns.f()` printing nothing in a later `use()`, where Jinja2 3.1.6 prints `1` (the verifier saw `1`,
`3` and `12` for its with and nested-loop variants). The macro's call context ends with the failing host call, so
InEndingModule does not see it.

**Proposal.** A frame of a call whose closures something outside the failing render can hold (a template module's
macro call, a host-swallowed nested render) closes over on failure too; find the predicate that covers both without
copying on every failure in a plain render.

**Done when.** Rows next to `FailureInATopLevelBodyKeepsTheClosureNames` in test/template_module_test.cpp, for a loop,
a `with` and a nested loop in a macro body, print Jinja2's values on each later call, narrow and wide.
