---
status: open
priority: medium
area: parity
touches: [src/statements.cpp, test/parity/divergences/statements.txt]
---
# A macro defined in a macro loses the outer call's names after that call returned

**Problem.** Found by the 0223 scenario suite (row F5). A macro defined inside another macro and
stored outside it (a namespace attribute) reads the outer macro's parameters and locals as
undefined once the outer call has returned:

```jinja
{% set ns = namespace(f=none) %}{% macro outer(n) %}{% macro inner(k) %}{{ n }}{{ k }}{% endmacro %}{% set ns.f = inner %}{% endmacro %}{{ outer('captured') }}|{{ ns.f('k') }}
```

Python Jinja2 3.1.6 prints `|capturedk`; Jinja2C++ prints `|k`. A top-level name read from such a
closure works, and so does a stored `caller` (0165). Pinned by
`statements.macro_closure_outlives_outer_call` in the corpus.

**Proposal.** The closure keeps the frame of the call that defined it, as 0165 did for macro
bodies run where they are defined. Check against 0301 (D2a gives a macro stored in a namespace
and called in a later call `TemplateExpired`; this case is called after the call, in the same
render) and 0049 P1's traversal of closures.

**Done when.** The corpus case matches and its divergence line is deleted; ASan clean.
