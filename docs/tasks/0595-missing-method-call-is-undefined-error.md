---
status: done
priority: low
area: parity
touches: [src/value_methods.cpp#ThrowNoAttribute]
shares: [python/tests/test_errors.py]
---
# Calling a missing method of a str, number or list raises TemplateRuntimeError, not UndefinedError

**Problem.** In Jinja2 3.1.6 `'a'.nope` is an `Undefined`, and calling it raises `UndefinedError`
(`'str object' has no attribute 'nope'`). Jinja2C++ raises the same message as a `TemplateRuntimeError`
(`methods::ThrowNoAttribute`, src/value_methods.cpp), so the Python module raises `jinja2cpp.TemplateRuntimeError`
for `{{ 'a'.nope() }}`, `{{ (1).nope() }}` and `{{ [].nope() }}`. `{{ x.nope() }}` of a dict is already an
UndefinedError. The parity corpus compares only the fact of an error, so `test/parity/` cannot pin it.

**Proposal.** Make the call of an attribute a value has not got go through the undefined value Jinja2 makes
(`UndefinedError` under every policy, as calling any Undefined does), keeping the message.

**Done when.** A `python/tests/test_errors.py` row raises `UndefinedError` for the three templates above, as Jinja2 does,
and the C++ error code is `ErrorCode::UndefinedError`.
