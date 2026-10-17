---
status: open
priority: low
area: parity
touches: [src/filters.cpp, src/global_functions.cpp, src/testers.cpp]
---
# Argument checks that 0200 left over

**Problem.** Found by the 0200/0127 verifier against Python Jinja2 3.1.6, each pinned by a
corpus case and a divergences line:
- `['a']|map('upper', 1)` renders; `map` drops the extra argument for a filter that takes none,
  where Python passes it on and `upper` raises TypeError (`filters.map_filter_extra_arg`).
- `range(stop=2)` renders; Python's `range` takes no keyword arguments
  (`globals.range_keyword_arg`).
- `{% set r = l|select('odd', 1) %}` raises, because `select` runs its test eagerly; in Python
  the generator is never read, so nothing raises (`tests.select_unread_extra_arg`). Same model
  as the deliberate 0101 divergence; fixing it needs a lazy `select`.
- The comparison tests take their argument by keyword (`1 is eq(b=1)`), which Python's
  `operator.eq` refuses (no corpus case yet).

**Proposal.** Have `map` build the named filter with the extra arguments so its own
`ParseParams` records the error; make `range` reject keyword arguments; mark the comparison
tests' argument positional-only. Decide whether the unread `select` case is worth a lazy
generator or should become a deliberate divergence like 0101.

**Done when** the three corpus cases match (or the `select` one is marked deliberate) and
their lines in `test/parity/divergences/` are deleted.
