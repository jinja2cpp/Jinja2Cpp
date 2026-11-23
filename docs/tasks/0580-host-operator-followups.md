---
status: open
priority: low
area: parity
depends: [0381]
touches: [src/value_visitors.h#BinaryMathOperation, src/filters.cpp, src/serialize_filters.cpp, python/src/convert.cpp, python/src/adapters.cpp]
shares: [test/public_host_object_test.cpp, python/tests/test_values.py]
---
# Host operators: the leftovers of task 0381

**Problem.** Task 0381 gives a host object its arithmetic and ordering operators through
`IHostObject::Operate`. The plan left these differences from Python Jinja2 3.1.6 out of it, each
small and independent:

- **`int`, `float` and `str` subclasses lose their methods.** The Python module converts them to
  engine scalars (`python/src/convert.cpp`), so a subclass that overrides `__add__` or `__lt__`
  is added or ordered as the base type.
- **`list` and `tuple` subclasses that override `__add__` or `__lt__`.** The engine takes its own
  list path first (`HostSequenceOperation`), so the override is never asked.
- **`|abs` and `|round`** do not ask `__abs__` and `__round__`. They need new operators, which
  after the API freeze means `IHostObject2` (`HostOperator` is append-only).
- **pprint's `_safe_key`** (`src/serialize_filters.cpp`) with host keys: Python orders them by
  `__lt__`, the engine does not.
- **`[1] + x`** says `unsupported operand type(s) for +: 'list' and 'T'` where Python says
  `can only concatenate list (not "T") to list`. An existing gap that 0381 did not widen.
- **Host-number cost.** `Fraction ** 10**8` costs what Python's does and is not covered by
  `MaxBigIntBits`; Jinja2's answer is `intercepted_binops`, which is task 0412.
- **A template macro as an operand** reaches the object as None (`ToHostValue` of a callable
  outside a call), where Python hands it the macro. Safe, since nothing of the template is kept,
  but a host whose `__add__` accepts None would answer.

**Done when.** Each item is fixed, or recorded on the divergence allow-list or in the module's
docs with the reason, by a regression row in `test/public_host_object_test.cpp` or the module's
`test_values.py` that renders as Python does under the pinned Jinja2.
