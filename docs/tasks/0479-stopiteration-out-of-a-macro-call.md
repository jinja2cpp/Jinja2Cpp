---
status: open
priority: low
area: parity
depends: [0477]
touches: [src/statements.cpp, python/src/adapters.cpp, python/src/environment.cpp]
shares: [python/tests/test_values.py, docs/python.md]
---
# A StopIteration out of a macro the template calls is undefined in Jinja2

**Problem.** Jinja2 calls a macro, `caller()` and every other callable of a template through
`Context.call`, which makes a call that raises `StopIteration` an undefined value. In the module only a
Python callable gets that (0434, `PyHost::Call`): a filter inside a macro that raises `StopIteration`
goes through the engine's macro call unchanged and ends the render as `RuntimeError: generator raised
StopIteration` (0477). Found by 0477.

```
{% macro m() %}{{ 1|stop }}{% endmacro %}[{{ m() }}]     Jinja2: "[]"   module: RuntimeError
```

`test_stop_iteration_from_a_call_is_undefined` in python/tests/test_values.py pins the module's answer.

Other Python callables the engine calls without `PyHost::Call` have the same gap (found by the 0477
verifier): `_`, `gettext`, `ngettext` and `{% trans %}` with `install_gettext_callables` raising
StopIteration (Jinja2: UndefinedError "value was undefined because a callable raised a StopIteration
exception"), and in a sandbox `'{a.p}'.format_map(d)` / `.format(...)` where the property `p` raises it,
or a `wrap_str_format` that does (Jinja2 renders `''`). `{% call m() %}` and `{{ m().x }}` follow the
macro case.

**Proposal.** Where the engine's call of a template callable (a macro, `caller`) fails with a host
exception, let the host say whether the failure is a "stop" (an `IHostObject`/`ISandboxPolicy`-free
query, for instance a flag the Python host sets on the exception it keeps) and give the undefined value
then, as `PyHost::Call` does. Check that a `StopIteration` from a filter outside any macro still
ends the render as 0477 has it, and that `{{ m().x }}` raises UndefinedError as Jinja2 does.

**Done when.** The template above renders `[]` through the module and the test asserts it for both
engines.
