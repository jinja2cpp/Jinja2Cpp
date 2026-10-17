---
status: done
priority: low
area: parity
depends: [0047]
touches: [src/serialize_filters.cpp#PrettyPrinter, test/filters_test.cpp]
---
# `pprint` prints JSON-style scalars

**Problem.** `PrettyPrinter` (`src/serialize_filters.cpp`) prints `None` as `none` and bools as
`true`/`false`, where Python's `pprint` prints `None`, `True` and `False` (found by the 0047
verifier: `{{ none|pprint }}`). Many unit-test rows encode the lowercase form, so the fix
edits them together with the printer.

**Done when.** `pprint` of `none` and of bools matches Python, and the unit rows that encode
the old text are updated.

**Resolution.** `pprint` prints Python's repr on one line: `None`, `True`/`False`, tuples as
`(a, b)` and `(a,)`, dictsort pairs as `('k', v)`, strings and dict keys with Python's quoting
and escapes, floats as Python's repr (`1.0`), macros as `<Macro 'm'>`. Wrapping at width 80 is 0215.
