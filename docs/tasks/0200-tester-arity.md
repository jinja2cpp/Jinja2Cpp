---
status: done
priority: low
area: parity
touches: [src/testers.cpp]
---
# Every built-in test checks its arguments

**Problem.** 0190 made `divisibleby`, `in` and `sameas` raise the argument error that
`ParseParams` records. The other tests still accept a call that does not fit, where Python
raises `TypeError`: the comparison tests (`1 is eq`, `1 is eq(1, 2)`, `lt`, `ge`, ...) and
`startingwith` ignore the recorded error, and the argument-less `ValueTester` modes
(`odd`, `defined`, `none`, `boolean`, `string`, ...) never parse their arguments, so
`1 is odd(1)` renders a result.

**Proposal.** Surface the argument error for every built-in test, as `ExpressionFilter`
does for filters (record it at Load, raise when the test runs): give `ITester` a
`GetArgumentsError()` and check it in `IsExpression::Evaluate` and wherever a test runs by
name (`select`, `reject`, `selectattr`, `rejectattr`); have the argument-less modes call
`ParseParams` with an empty table.

**Done when** `tests.eq_arity`, `tests.eq_extra_arg`, `tests.odd_extra_arg` and
`tests.defined_extra_arg` match and their lines in `test/parity/divergences/tests.txt` are
deleted.

**Done** (PR TBD). Comparator and `startingwith` raise the recorded argument error when the
test runs, and the argument-less `ValueTester` modes parse their call against an empty table,
so `1 is odd(1)` and `select('odd', 1)` raise. Checked in each `Test`, which covers `is` and
the tests that `select`, `reject`, `selectattr` and `rejectattr` name. Leftovers (keyword
argument to the comparison tests, unread `select`, `map` extra arguments) are in 0206.
