---
status: open
priority: medium
area: parity
depends: [0012]
touches: [include/jinja2cpp/value.h, src/internal_value.h, src/internal_value.cpp]
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

**Decision (Ruslan, 2026-10-24).** Option 1: add a `float` alternative to `Value` and print it
with the shortest repr that round-trips a `float`. It changes the public ABI, so it belongs to
the 2.0 API wave and must land before the G1 freeze (docs/release-2.0.md, D21). Two notes for
the implementer:
- 0180 found that `MakeCallable` visits every argument combination. Each new `Value`
  alternative multiplies its instantiations (15^N for N arguments). Move it to a per-argument
  visit first, or in the same PR.
- In templates, a `float` behaves as a Python float (arithmetic widens to `double`). Only its
  printed form and its round trip back to C++ differ.

**Done when.** `{{ x }}` with `x = 12.123f` prints `12.123`, for a `Value` and for a reflected
`float` member; a `float` read back through `Value` keeps its type; the tests that encode the
widened digits (`test/expressions_test.cpp`) are updated; the abidiff report shows the
intended layout change only.
