---
status: open
priority: low
area: parity
depends: [0434]
touches: [src/value_methods.cpp, src/internal_value.cpp]
shares: [test/parity/cases/literals.py, test/parity/divergences/literals.txt]
---
# `x.__class__` of the engine's values

**Problem.** In Jinja2 `{{ 'a'.__class__.__name__ }}` is `str`, and `{{ foo().__class__.__name__ }}` of a
macro's result is `Markup` or `str` by the autoescape in force. The engine's values have no
`__class__`, so the attribute is undefined and its `__name__` an UndefinedError
(`literals.class_name_of_str`, rejects). Found by 0434 through the upstream suite
(`test_ext.py::TestAutoEscape::test_volatile_scoping`, which also needs `Environment.compile` and so
stays listed as unsupported).

**Proposal.** A read-only `__class__` for the engine's values that names Python's type
(`str`, `Markup`, `int`, `float`, `bool`, `NoneType`, `list`, `tuple`, `dict`, `range`, ...): an object
with `__name__` and `__qualname__` that prints as `<class 'str'>`, compares equal to itself and to
nothing else, and is not callable. The sandbox refuses it as Jinja2's does (a name with an
underscore). Host objects keep the host's own attribute.

**Done when.** `literals.class_name_of_str` matches and its divergence line is gone; a sandboxed
render still refuses `__class__`.
