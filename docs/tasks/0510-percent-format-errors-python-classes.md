---
status: open
priority: low
area: parity
depends: []
touches: [src/python_format.cpp]
shares: [python/tests/test_i18n.py]
---
# `%` formatting errors carry no Python class

**Problem.** `PythonPercentFormat` (src/python_format.cpp) throws a plain `TemplateRuntimeError` at
each of its 25 error sites, without the `TemplateRuntimeError::PythonClass` that names the Python
exception. Python raises ValueError (`incomplete format`, `incomplete format key`, `unsupported
format character`) or TypeError (`not enough arguments for format string`, `format requires a
mapping`, `%d format: a real number is required`). So the Python module raises
`jinja2cpp.exceptions.TemplateRuntimeError` where Jinja2 raises ValueError or TypeError: `{{ t % {'a': 1} }}`
with `t='%(a'`, and newstyle `gettext(t, a=1)`, which formats with the same code. Found by 0415:
`python/tests/test_i18n.py::test_formatting_a_translation_checks_what_fits_in_one_pass` pins the
current class with a comment naming this task.

**Proposal.** Pass the class at each throw site, as `CallOldGettext` in src/global_functions.cpp
does; check the parity corpus's error cases (`format` filter and `%` operator areas) still match.

**Done when.** The Python module raises ValueError/TypeError for those templates as Jinja2 does,
and the test_i18n.py row expects ValueError again.
