---
status: open
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
