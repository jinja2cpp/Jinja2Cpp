---
status: open
priority: low
area: filters
depends: []
touches: [src/filters.cpp]
shares: []
---
# attr() of a method renders empty

**Problem.** `{{ 'abc'|attr('lower') }}` renders an empty string; Jinja2 prints the bound
method (`<built-in method lower of str object at 0x...>`), and `{{ ('abc'|attr('lower'))() }}`
gives `abc`. Methods on values (0020) are reachable through `.` and `[]` but not through
`attr`. Found by the 0180 verifier.

**Proposal.** Let `attr` look up the value's method table as attribute access does, so
`x|attr('lower')` returns the same callable as `x.lower`; the printed form can stay a
documented divergence (the address is not stable anyway).

**Done when.** `{{ ('abc'|attr('lower'))() }}` renders `abc` (parity case in
`test/parity/cases/methods.py`), and `x|attr('missing')` stays undefined.
