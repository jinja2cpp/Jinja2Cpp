---
status: in-progress
priority: low
area: parity
depends: [0432]
touches: [src/statements.cpp, python/src/errors.cpp, python/jinja2cpp/loaders.py]
shares: [test/includes_test.cpp, python/tests/test_errors.py]
---
# Include and constructor errors with Jinja2's classes

**Problem.** After 0432 these still raise another class or message than Jinja2 3.1.6 (found by the
0432 verifier):

- `{% include ['x'] %}` of a missing template raises TemplateNotFound; Jinja2 raises
  TemplatesNotFound (a list always goes to `select_template`). The engine reports the names as a list
  for a string name too, so the module cannot tell them apart.
- `{% include [] %}`: Jinja2 says "Tried to select from an empty list of templates."; the module's
  message is `None`.
- `{% include 5 %}` is a TypeError in Jinja2, `{% include none %}` TemplatesNotFound; here both are
  TemplateRuntimeError.
- A loader whose `get_source` raises `TemplateNotFound(name, "custom message")` (a
  `FileSystemLoader` subclass, or any Python loader) loses its message: the module raises
  `'x' not found in search path: ...` for a FileSystemLoader subclass and the bare name for others,
  where Jinja2 passes the loader's own exception through (0436 verifier round 2).
- `{{ dict(5) }}` is a TypeError and `{{ [1]|xmlattr }}` an AttributeError in Jinja2; here
  TemplateRuntimeError.

**Proposal.** Mark a list name in TemplateNotFound's ExtraParams (for example ExtraParams[1] =
"list"), report the empty list and the bad name types with Jinja2's class through ExtraParams[1] of
TemplateRuntimeError (0432's contract, which knows TypeError and ValueError; add AttributeError),
and map them in the module. Add the cases to test/includes_test.cpp and python/tests/test_errors.py.

**Done when.** The cases raise Jinja2's classes and messages through the module.

**Result (parity batch 7).** An include's TemplateNotFound marks a list of names with ExtraParams[1]
`"list"` (TemplatesNotFound in the module, also for one name); `none` and `[]` select from an empty list
("Tried to select from an empty list of templates.", skipped by `ignore missing`); a name that is neither
a string nor iterable is TypeError `'int' object is not iterable` with or without `ignore missing`; an
undefined name fails as used. `dict()` raises Python's TypeError/ValueError messages (element index
included) and UndefinedError for an undefined argument; `xmlattr` of a non-mapping is AttributeError
(`TemplateRuntimeError::PythonClass::AttributeError`, new). A Python loader's own TemplateNotFound is
kept by the module's SourceHandler and raised as is for that name (include and get_template). Left: an
undefined name inside the list (0492).
