---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1005
priority: low
area: parity
depends: [0352]
touches: [include/jinja2cpp/host_object.h, src/host_object.h, src/host_object.cpp, src/public_host_object.cpp, src/value_visitors.h#BinaryMathOperation, src/value_visitors.h#UnaryOperation, src/string_converter_filter.cpp, src/filters.cpp, src/testers.cpp, src/expression_evaluator.cpp, src/stable_sort_values.h, python/src/adapters.h, python/src/adapters.cpp]
shares: [test/public_host_object_test.cpp]
---
# Host objects: arithmetic and ordering operators

**Problem.** `IHostObject` (0352) has `Equals`, but no entry for the other operators.
The engine therefore answers every `+ - * / // % **`, every ordering (`< <= > >=`) and
`odd`/`even` (`x % 2`, since 0362) on a host object with Python's TypeError:
"unsupported operand type(s)" or "'<' not supported between instances". That is correct
for a plain object. The Python module (0248) also passes objects that define these
methods, though: `decimal.Decimal`, `datetime` values with `timedelta`, `fractions`,
numpy scalars, and user classes with `__lt__` for `sort` and `max`. Python Jinja2 calls
those methods.

**Proposal.** Give the seam one binary-operator entry. It would take the operator and the
other operand, and say which side the object is on, so that `__radd__` and friends work.
It returns nullopt for NotImplemented. The engine then tries the other operand's
reflected method, then raises the TypeError it raises today. Ordering inside `sort`,
`min`, `max` and `dictsort` should use the same entry (`__lt__`). Results come back as
values the template can read, which may be other host objects.

**Done when.** On the toy (`test/public_host_object_test.cpp`), an object with `__add__`,
`__radd__`, `__mod__` and `__lt__` renders as an equivalent Python class does under the
pinned Jinja2, narrow and wide. This covers `x + 1`, `1 + x`, `x is odd`, `[x, y]|sort`
and `x < y`. An object without them keeps today's errors.

**Plan.** Approved by Ruslan 2026-11-23 (one `IHostObject::Operate(HostOperator, other,
HostOperandSide)` entry returning `optional<Value>`, the Python module on the full
`PyNumber_*`/`PyObject_RichCompare` protocol, unary `-`/`+` in scope). Three PRs:

- P0: `StableSortValues` becomes a guarded merge sort. libstdc++'s `std::stable_sort`
  writes below the range when a comparator answers inconsistently, which a host `__lt__`
  reaching `{{ items|sort }}` could do.
- P1: the seam (`HostOperator`, `HostOperandSide`, `IHostObject::Operate`), the engine dispatch
  (`+ - * / // % **`, orderings, unary, `odd`/`even`, `min`/`max`/`dictsort`/`groupby`, left
  then reflected right; `unique` never asks for an order) and the toy tests. Follow-ups that
  stay out of it: task 0582.
- P2: the Python module bridge: `PyHost::Operate` runs `PyNumber_*` and `PyObject_RichCompare` for every object that
  is not an exact dict, list or tuple (Python's own `TypeError` propagates), with sequence-cap guards before
  `Multiply` and `Add` for types that repeat or concatenate (`bytes`, `bytearray`, `array`, `deque`; by the storage a
  subclass repeats, bounded by a deque's `maxlen`) and before `Remainder` for printf-style padding; `docs/python.md`,
  "Operators on Python objects".
