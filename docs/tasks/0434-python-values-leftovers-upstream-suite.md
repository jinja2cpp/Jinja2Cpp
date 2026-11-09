---
status: open
priority: low
area: parity
depends: [0355]
touches: [python/src/adapters.cpp, python/src/convert.cpp, python/src/template_module.cpp, src/filters.cpp]
shares: [python/tests/upstream/expected-failures.txt]
---
# Values, filters and macros: leftovers the upstream Jinja2 suite found

**Problem.** Lines of `python/tests/upstream/expected-failures.txt` (0355) where jinja2cpp
renders or behaves differently from Jinja2 with Python data:

- `ChainableUndefined`: `foo.bar["baz"]._undefined_name` is the first missing step (`foo`,
  `bar`, `baz`); jinja2cpp gives empty (`test_api.py::TestUndefined::test_chainable_undefined`).
- A callable that raises StopIteration gives an undefined value in Jinja2 (`A{{ test() }}B` is
  `AB`); jinja2cpp lets the StopIteration out (`test_stopiteration_is_undefined`).
- `{{ foo().__class__.__name__ }}` of a macro's result is `Markup` or `str`; the engine has no
  `__class__` on its strings (`test_ext.py::TestAutoEscape::test_volatile_scoping`; the test also
  needs `Environment.compile`, so it stays listed as unsupported after this).
- `int` of an object with `__int__` is 42, not 0 (`TestFilter::test_int_special_method`).
- `pprint` is Python's `pprint.pformat` (wraps at 80 columns) (`test_pprint`).
- `random` draws from Python's `random` module, so `random.seed` reproduces it (`test_random`).
- `complex is number` is true (`test_tests.py::TestTestsCase::test_types[complex is number-True]`).
- A module macro has `name`, `arguments`, `defaults`, `catch_kwargs`, `catch_varargs` and
  `caller` attributes (`test_core_tags.py::TestMacros::test_macro_api`), and takes an
  EvalContext as its first argument, as Jinja2's Macro does (`test_security.py::TestSandbox::test_template_data`).

**Proposal.** Fix them in the crossing (adapters, conversion, module macros) where Python data
is involved, and in the engine where it is not (`pprint`, `__class__`). `random` through the
module can call `random.choice`; the C++ engine keeps its generator (record as deliberate if so).

**Done when.** The listed upstream tests pass through the module, or their lines become
`deliberate` with the reason.
