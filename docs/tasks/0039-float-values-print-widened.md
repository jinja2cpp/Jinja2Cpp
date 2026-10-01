---
status: open
priority: low
area: parity
depends: [0012]
touches: [include/jinja2cpp/value.h]
shares: [src/value_visitors.h]
---
# C++ `float` values print with their widened digits

**Problem.** Since 0012, numbers print as Python `repr()` does: the shortest form that
round-trips a `double`. `jinja2::Value` has no `float` alternative, so a C++ `float` is
widened to `double` when the `Value` is built, and `Value(12.123f)` prints
`12.123000144958496`. That is the correct repr of the stored double, so templates fed
from Python-like data are unaffected. For an embedder who passes `float` fields, though
(reflected structs with `float` members, for example), the output is noisier than before,
when 8 significant digits hid the widening. The unit tests that pass `12.123f` encode
this (`test/expressions_test.cpp`).

**Proposal.** Pick one:
- add `float` to the `Value` variant and print it with the shortest repr that round-trips
  a `float` (`12.123`). This changes the public ABI, so it waits for a major version;
- make reflection of `float` members round to the shortest `float` repr before widening,
  which keeps the ABI but changes only the reflected path;
- document the behaviour and recommend `double` (the README changelog already does this).

**Done when.** The decision is recorded here, and `{{ x }}` with `x = 12.123f` prints
`12.123` if a code option is chosen.
