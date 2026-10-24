---
status: open
priority: low
area: parity
depends: []
touches: [src/testers.cpp]
---
# `divisibleby` on a value that is not a number

Found by the 0045/0291 verifier. Jinja2's `test_divisibleby(value, num)` is `value % num == 0`,
so it inherits Python's `%`: `'a' is divisibleby 2` raises `TypeError` (str formatting with
no format spec), `'%d' is divisibleby 2` formats to `'2'` and is False, and a list or None
raises. Jinja2C++ answers False for every non-number.

Case: `tests.divisibleby_string`.

**Proposal.** Compute the test with the evaluator's `%` (`BinaryExpression::DivRemainder`,
which already implements str formatting and the TypeErrors) and compare the result with 0.

**Done when.** No line of `test/parity/divergences/` names task 0305, and `ctest -R parity` passes.
