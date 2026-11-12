---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/850
priority: low
area: parity
depends: [0355]
touches: [python/src/adapters.cpp, python/src/template_module.cpp, src/filters.cpp, src/serialize_filters.cpp, src/undefined.cpp, src/testers.cpp, src/expression_evaluator.cpp, src/template_module.cpp, src/public_host_object.cpp, src/host_object.h, include/jinja2cpp/host_object.h, include/jinja2cpp/template.h, include/jinja2cpp/generic_list_iterator.h]
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

**Progress (2026-11-11).** All nine lines are closed: seven tests pass through the module, `random` is
`deliberate` (the engine keeps its own generator; Python's `random.seed` cannot reach it) and
`test_volatile_scoping` is `unsupported` (`Environment.compile`), its `__class__` gap filed as 0476.
- StopIteration from a call is the environment's undefined value (`PyHost::Call`, Jinja2's
  `Context.call`). A filter's StopIteration still ends the render, but Jinja2 raises RuntimeError
  there: 0477.
- A module macro takes an EvalContext first argument (`jinja2cpp.nodes.EvalContext`, a partial
  `jinja2.nodes`, or Jinja2's), whose `autoescape` decides Markup.
- `_undefined_name` and `_undefined_hint` are attributes of every undefined value outside the sandbox
  (`UndefinedField`); a ChainableUndefined keeps the first missing step. Pinned by
  `undefined.*undefined_name*` cases. An int key comes back as an int; other non-string keys as their repr.
- `pprint` reads an exact host `list` or `tuple` through its iteration, so it breaks and sorts as
  `pformat`; a subclass keeps printing as its repr (0453 stays open for subclasses that keep `list.__repr__`) (`filters.pprint_wraps_context_list`
  through the module's corpus run).
- Before the 2.0 freeze: `IHostObject::IsNumber`, `ToInt`, `ToFloat` (`is number`, `int`, `float`) and
  `jinja2::MacroAttributes(UserCallable)` for the module's macro attributes (docs/api-2.0.md, section 7).
- Found on the way and fixed: an empty `GenericList` with an accessor never reached `end()`.
- Left as they are: `d['']._undefined_name` is None (an empty name means none); a list
  that contains itself ends in the size cap or RecursionError, where pformat prints
  `<Recursion on list ...>`.
