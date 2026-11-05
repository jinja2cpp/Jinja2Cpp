---
status: open
priority: low
area: api
depends: [0359]
touches: [src/token_budget.cpp, src/token_budget.h]
---
# Running token budget across a render

Filed by the 0358/0359 plan (Ruslan 2026-11-05).

**Problem.** `pack` sees only its own budget; it cannot know how many tokens the rest of the
template has written. A host approximates it with `reserve`.

**Proposal.** A render-scoped budget object (a global such as `budget()` with `used`/`left`, or a
`pack` mode) that counts what the template writes. Counting the output means tokenizing it, so
the design must show the cost is paid only by templates that use it.

**Done when.** A design is agreed (new API) and, if implemented, tests show the budget across two
`pack` calls and fixed text.
