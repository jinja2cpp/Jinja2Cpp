---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1020
priority: low
area: robustness
depends: [0543]
touches: [src/value_visitors.h, src/filters.cpp, src/internal_value.cpp]
---
# Render a huge integer used as a list index or a range() bound as Python does

**Problem.** Found by the verifier on the 0543 P2 PR, in code that PR does not touch. With `h`
an integer past 4300 digits (for example `('f' * 3600)|int(base=16)`), pinned Jinja2 renders
`{{ [1, 2, 3][h] }}` as empty: the list raises IndexError ("cannot fit 'int' into an
index-sized integer"), which Jinja2's `getitem` turns into an undefined. Jinja2C++ fails the
render with ValueError instead, because building the undefined's message prints the key
through `CheckedDecimal`, which refuses past 4300 digits. `range(h)|first` also fails in
Jinja2C++ where Python renders `0` (a lazy range takes an arbitrarily large bound).

**Proposal.** Name a huge key in an undefined's message without its digits (Python's message
never prints it), and let `range()` accept a bound past int64 when it is only read lazily, or
record the second as a deliberate divergence. Add parity cases with `h` built from a context
string (not folded, see 0564).

**Done when.** Both expressions render as pinned Jinja2 renders them, narrow and wide, or the
range case is on a divergences list with its reason.

**Progress.** Done-ready. `MakeUndefined` (src/undefined.cpp) names an integer key past 4300
digits as `<integer of N bits>` instead of printing it through `Repr`, which raised ValueError;
small keys keep `list object has no element 5`. `[1,2,3][h]`, `[1,2,3][-h]`, tuple, string and
dict subscripts now render empty as in Python (`subscripts.huge_index_context`, narrow and
wide). `range(h)|first` stays a deliberate divergence (`globals.range_huge_context_bound`, rejects):
clamping the stop would make `|length` and the repr wrong where Python raises OverflowError or
prints the bound, and ranges are int64 by 0178.
A key that holds such an integer (`[1, 2, 3][(h, 1)]`, `{'a': 1}[(h,)]`, `(1, 2)[[h]]`) cannot be
printed either, so the undefined's message names it `<key that cannot be printed>` (Python formats
its message only when it is read): parity case `subscripts.huge_index_nested_context` (verifier round 1).
