---
status: open
priority: low
area: parity
depends: [0477]
touches: [include/jinja2cpp/exception.h, include/jinja2cpp/fwd.h, src/error_info.cpp, src/call_stop.h, src/call_stop.cpp, src/render_failure.h, src/out_stream.h, src/out_stream.cpp, src/generation_tracker.h, src/render_context.h, src/expression_evaluator.cpp, src/statements.cpp, CMakeLists.txt, python/src/errors.h, python/src/adapters.cpp, test/macro_test.cpp, test/template_module_test.cpp, test/chat_template_preset_test.cpp, docs/api-2.0.md]
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

## Implementation

The host marks an exception with the public mix-in `jinja2::StopIteration` (exception.h; approved by Ruslan
2026-11-13, recorded in docs/api-2.0.md 2.1). The engine's call sites (`CallExpression::RenderCallable` and
`CallCallable`, which also cover `caller()`, `loop(...)` and host callables; `MacroCallStatement::Render`, which
gives the 0362 `TemplateRuntimeError`) guard only the invocation, in the shape of the body loops (src/call_stop.h).
A stop is a `RenderFailure` that no seal covers (`SealStop`, set by the body of an included, imported or extended
template and by `RenderBlockAt`, which `self.b()`, `super()` and blocks go through), a `HostCallableFailure` or a raw
exception that holds a marked one. The call then leaves its context as it found it (`HomeCall::Snapshot`), takes its
output back (`OutStream::Mark`/`RollbackTo`, `GenerationFreeze::Discard`) and gives `MakeStopUndefined`. A sink that
was given the output since the call began cannot take it back and the failure goes on. The module marks Python's
`StopIteration` (`PyStopIteration`, `CallPython` and `ThrowPending`). Follow-ups: exact rollback for sinks (spill
while a macro holds the piece), and gettext/ngettext/trans and the sandbox's format/format_map/wrap_str_format.
