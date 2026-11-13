---
status: in-progress
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

**Resolution** (PR pending, branch `claude/project-thread-rhjf86`). `MakeClassObject`
(internal_value.cpp) gives the class of a value the engine made: an object of class `type` with
`__name__`, `__qualname__` and `__module__`, printed as `<class 'str'>` (`<class 'markupsafe.Markup'>`
for a module other than builtins). Its identity is the class's entry in a fixed table, so two reads
are equal, `sameas` and the same dict key, and it equals nothing else. `x.__class__`, `x['__class__']`,
`x|attr('__class__')` and `'{0.__class__}'.format(x)` reach it; an undefined has its policy's class
(`jinja2.runtime.StrictUndefined`, ...). A dict's own `__class__` key stays its item (`d['__class__']`)
while `d.__class__` is the class, as in Python. Not covered: host objects (their own attribute), maps
whose keys come first (reflected structs), a value whose Python class the engine does not name,
dict views (0489, `list` here), and calling a class (`'a'.__class__()`, Python's constructor; it
fails as calling a missing attribute). With a sandbox policy the policy judges the name first, as
for any attribute (public_sandbox_policy_test.cpp). Cases: `literals.class_*`.
