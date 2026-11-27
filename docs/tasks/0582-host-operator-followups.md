---
status: in-progress
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
- **`'%d' % host_number`** (`np.int64`, `Fraction`, a class with `__index__`) raises "%d format: a real number is
  required, not dict" where Jinja2 renders the number: the engine's `PercentFormat` hands a host object over as a
  mapping. `%f` with `__float__` fails the same way; `%s` works.
- **The guard calls `__len__` and `__index__` before Python does**, so a `bytes * n` with
  user-defined `__index__` runs it twice.

**Done when.** Each item is fixed, or recorded on the divergence allow-list or in the module's
docs with the reason, by a regression row in `test/public_host_object_test.cpp` or the module's
`test_values.py` that renders as Python does under the pinned Jinja2.

## Outcome

Fixed, each with a regression row (`test/public_host_object_test.cpp`: `ListConcatenationOfADecliningObjectNamesTheLeftType`,
`PercentFormatReadsAHostNumberAsPythonDoes`, `AbsAndRoundAskTheObject`, `PprintOrdersHostKeysByTheirOwnLessThan`;
`python/tests/test_values.py`: the `CASES` rows marked task 0582 and the `TYPE_ERRORS` rows for `[1] + d`, `(1, 2) + d`,
`'%d' % k` and `'%f' % k`, all compared with Python Jinja2 3.1.6):

- `[1] + x` and `(1, 2) + x` say `can only concatenate list (not "T") to list` (tuple likewise).
- `'%d'`, `'%i'`, `'%u'`, `'%x'`, `'%o'`, `'%f'`, `'%e'`, `'%g'` of a host number, alone or as a tuple item, render the
  number (`IHostObject::ToInt`, `ToFloat`); the error of a host that is no number names its type, not `dict`.
- `|abs` and `|round` (method `common`) ask the host through `HostOperator::Absolute` and `HostOperator::Round`
  (appended); the Python module runs `abs()` and `round(x, n)`. `ceil` and `floor` are recorded, not fixed: Python runs
  `math.ceil(x * 10**n) / 10**n` on the object, which needs the host's `*`, `**` and `/` composed in the filter.
- pprint orders dict keys that are host objects by the host's `<`, falling back to the type-name order as Python does.
- The Python module names a static C type by its module (`decimal.Decimal`), as the TypeError of Python's own operators
  does (`PyHost::TypeName`); the test suites of both modules are unchanged.

Not fixed: `Markup * x` with an `__index__` object. The repeat check is `CheckMarkupRepeatCount` in
`src/expression_evaluator.cpp`, which this task's file list does not include; the fix is to ask the host (`host::Operate`
with `HostOperator::Multiply`, the host on the right) before that check and mark a string result as Markup.

Recorded in `docs/python.md` ("Operators on Python objects"): `int`, `float` and `str` subclasses, `list` and `tuple`
subclasses that override operators, a macro as an operand, the `thread_local` native-order scope, the dropped Markup flag,
the doubled `__len__`/`__index__` call, and the host-number cost (task 0412), plus the differences of the new `%` and
`round` paths.
