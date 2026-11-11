---
status: done
priority: low
area: parity
touches: [src/value_methods.cpp]
shares: [test/parity/divergences/methods.txt]
---
# str.expandtabs is missing from the string methods

**Problem.** `'a\tb'.expandtabs(4)` fails with "'str object' has no attribute 'expandtabs'";
Python Jinja2 renders `a   b`. Found by 0263, which unified the str-method width cap and had
no `expandtabs` to cap. Pinned by the corpus case `methods.str_expandtabs` and its line in
`test/parity/divergences/methods.txt`.

**Proposal.** Add `expandtabs(tabsize=8)` next to the other padding methods in
src/value_methods.cpp, as Python's: columns reset at `\n` and `\r`, a tab size of 0 or less
removes tabs, columns count code points. The result size goes through `CheckSequenceSize`
before it is built (0263's rule), so `('\t' * 1000).expandtabs(2**20)` fails with
`ResourceLimitExceeded`.

**Done when** the corpus case passes in narrow and wide runs, its divergence line is gone, and a
`RecursionLimitsTest.GrowthOperationsCheckSizeFirst` row covers the huge tab size.

**Resolution.** `expandtabs(tabsize=8)` on str and Markup, counting code points; a first pass
sizes the result through `CheckSequenceSize` before anything is built. Corpus cases
`methods.str_expandtabs*`; `RecursionLimitsTest.GrowthOperationsCheckSizeFirst` rows for a
huge tab size (narrow and wide) and for many tabs.
