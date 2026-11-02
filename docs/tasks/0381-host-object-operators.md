---
status: open
priority: low
area: parity
depends: [0352]
touches: [src/host_object.h, src/value_visitors.h#BinaryMathOperation]
shares: [test/host_object_test.cpp]
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

**Done when.** On the toy (`test/host_object_test.cpp`), an object with `__add__`,
`__radd__`, `__mod__` and `__lt__` renders as an equivalent Python class does under the
pinned Jinja2, narrow and wide. This covers `x + 1`, `1 + x`, `x is odd`, `[x, y]|sort`
and `x < y`. An object without them keeps today's errors.
