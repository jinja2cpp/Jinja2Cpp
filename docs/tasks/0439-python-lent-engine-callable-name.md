---
status: open
priority: low
area: parity
depends: [0357]
touches: [python/src/convert.cpp, src/internal_value.cpp]
shares: [include/jinja2cpp/user_callable.h]
---
# Python module: an engine callable lent to Python prints without its name

**Problem.** Since 0357 S3 the module reads the engine's callables only through the public API.
A built-in global such as `range`, `lipsum` or `dict`, passed to a Python callable or read
through its context, arrives as a `LentCallable` with no name, so Python sees a
`TemplateValue` that prints as `<Macro>`. Python Jinja2 passes its own objects:
`repr(range)` is `<class 'range'>`. On master before S3 the private seam knew the engine's
globals and printed them as Jinja2 does. Probe:
`Template('{{ f(range) }}').render(f=repr)` gives `<class 'range'>` in Jinja2 and `<Macro>`
through the module. Calling the value works (`python/tests/test_context.py`,
`test_a_global_function_read_through_the_context_is_callable`).

**Proposal.** Either the module maps the engine's default globals to Python's own objects
(`range` to `builtins.range`, `dict` to `dict`, and so on), when a lent callable is one of them,
or the public `UserCallable` gains a name the engine sets for its built-ins. The first option
needs a public way to recognise the engine's built-in; the second is a public-API addition and
needs a decision before the G1 freeze.

**Done when.** The probe prints as in Jinja2 and a row in `python/tests/test_context.py`
checks it.
