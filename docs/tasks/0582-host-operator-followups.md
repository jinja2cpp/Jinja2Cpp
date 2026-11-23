---
status: open
priority: low
area: parity
depends: [0381]
touches: [src/value_visitors.h#BinaryMathOperation, src/host_object.cpp, src/public_host_object.cpp, src/filters.cpp, src/serialize_filters.cpp, python/src/convert.cpp, python/src/adapters.cpp]
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
  `can only concatenate list (not "T") to list` (and `(1, 2) + x` the same for tuple). An
  existing gap that 0381 did not widen.
- **Host-number cost.** `Fraction ** 10**8` costs what Python's does and is not covered by
  `MaxBigIntBits`; Jinja2's answer is `intercepted_binops`, which is task 0412.
- **A template macro as an operand** reaches the object as None (`ToHostValue` of a callable
  outside a call), where Python hands it the macro. Safe, since nothing of the template is kept,
  but a host whose `__add__` accepts None would answer.

- **The native-order scope is per thread.** `unique` holds a `thread_local` flag
  (`src/host_object.cpp`) under which host orderings decline; a host `__eq__` that renders a
  template sorting host objects during that window gets a TypeError. Move it into the render's
  own state, or document it.
- **The Markup flag of an operand is dropped** for `x + m`, `m + x`, `x < m` and `max`: the host
  gets a plain `str` (as `Equals` does), so `__add__` sees `str` where Python passes `Markup`.
  Errs toward more escaping, never less.

- **`Markup * x` with an `__index__` object** (`np.int64`, a user class) raises a TypeError
  where markupsafe repeats the text; the engine's Markup repeat check runs before the host is
  asked. Its message also names `'Decimal'` where Python names `'decimal.Decimal'`.
- **The guard calls `__len__` and `__index__` before Python does**, so a `bytes * n` with
  user-defined `__index__` runs it twice.

**Done when.** Each item is fixed, or recorded on the divergence allow-list or in the module's
docs with the reason, by a regression row in `test/public_host_object_test.cpp` or the module's
`test_values.py` that renders as Python does under the pinned Jinja2.
