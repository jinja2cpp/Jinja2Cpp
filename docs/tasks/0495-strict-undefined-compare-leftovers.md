---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/923
priority: low
area: parity
depends: [0484]
touches: [src/testers.cpp, src/value_methods.cpp, src/expression_evaluator.cpp]
shares: [test/parity/cases/undefined.py, test/parity/divergences/undefined.txt]
---
# StrictUndefined in the tests and methods that compare

**Problem.** 0484 made `==` of lists, tuples and dicts and `in` compare their items as Python does,
refusing a StrictUndefined item (with Python's identity shortcut). Its verifier found the paths
around it that still differ from Jinja2 3.1.6 under StrictUndefined (all as before 0484):

- `x in []` raises in the engine; Python gives False (nothing to compare);
- `{% set a = x %}{{ a in [a] }}` raises in the engine (the operator checks its left operand);
  Python gives True by identity (`a is in [a]` already does);
- `x is sameas x` is True in the engine, False in Python (two lookups, two objects), and
  `select("sameas", x)` differs the same way;
- `select("equalto"/"eq"/"ne", ...)` and `x is eq(...)` with an undefined item do not raise in the
  engine; Python does;
- `[x, 1].count(1)` and `.index(1)` do not raise in the engine; Python does;
- `x is iterable` is False in the engine; Python's StrictUndefined raises (and the default
  Undefined is iterable: True).

**Proposal.** Route these through the item comparison of 0484 (CheckItemsCompare in
src/undefined.h) and the operand check of `in`; add a case per bullet to
`test/parity/cases/undefined.py`.

**Done when.** The cases match in narrow and wide runs.

**Result (batch 8 A).** `in` no longer checks its left operand up front: the container compares (with
the identity shortcut) or hashes it (dicts, keys and items views), so `x in []` is False and `a in [a]`
True; comparisons check their operands after both are evaluated, as Python's do. `sameas` of two undefined
values is their identity; `eq`, `ne` and the ordering tests refuse undefined operands as operator.eq and
the others do; list `index`, `count` and `remove` compare as PyObject_RichCompareBool; `x is iterable` of
an undefined value is True, and StrictUndefined refuses it. Cases in `test/parity/cases/undefined.py`.
