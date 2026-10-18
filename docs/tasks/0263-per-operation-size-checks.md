---
status: open
priority: high
area: robustness
depends: [0262]
touches: [src/value_methods.cpp, src/value_visitors.h, src/python_format.cpp, src/global_functions.cpp, src/string_converter_filter.cpp, test/recursion_limits_test.cpp]
shares: [src/filters.cpp, src/internal_value.h]
---
# Every growth operation checks its size before allocating

**Problem.** `MaxSequenceSize` (0097) fires only where an operation calls `CheckSequenceSize`.
Measured on master 8b46d2d (Debug): `~` doubling builds 2^27 characters; `'x'.center(2**27)`
passes because the str-method width cap (2^28) is above the sequence cap; `('%(a)1000000s'*100)
% {...}` builds 100 MB; `lipsum(300000)` builds 134 MB; list `+` and `extend` doubling end in
`std::bad_alloc` after 16-27 s. Under 0250 R2' `bad_alloc` must reach the host only on real system
out-of-memory, so a template must never be able to cause one.

**Proposal.** Check the result size before allocating in: list `+`, `extend`, `insert`, `*`;
string `~` and `+`; `join`; `replace`; `%` and `format` totals; str-method widths (one cap,
unified with `MaxSequenceSize`); `lipsum`; `wordwrap`; string `|length` without building a list.
Map `std::length_error` to `ResourceLimitExceeded` (0250 P3). Review rule from now on: a new growth
operation calls `CheckSequenceSize` before it allocates.

**Tests.** Rows in `HugeSequencesFailFast` (test/recursion_limits_test.cpp) with a small
`maxSequenceSize` (0262) so they run fast: `ns.l + ns.l` doubling, `l.extend(l)`, `ns.s ~ ns.s`,
`'x'.center(2**27)`, `('%(a)1000000s'*100) % {'a': 1}`, `lipsum(300000)`, `range(N)|join`. Each
fails with `ResourceLimitExceeded` (`UnexpectedException` until 0250 P3), never `bad_alloc`.

**Done when.** The rows pass and the limits-on fuzz target (0267) reports no out-of-memory.
