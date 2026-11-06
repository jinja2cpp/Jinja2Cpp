---
status: open
priority: low
area: api
depends: [0354]
touches: [python/src/adapters.cpp, python/jinja2cpp/sandbox.py, src/host_object.h, src/undefined.cpp, src/internal_value.cpp]
shares: [python/tests/test_sandbox.py, docs/python.md]
---
# Sandbox policy on the engine's own values

**Problem.** The sandbox of the Python module (0354) asks the environment's `is_safe_attribute`
for every attribute of a Python object. The values the engine makes itself (the template's `str`,
`int`, `list`, `dict`, `loop`, `cycler`, macros) have no Python attributes, so the module treats a
missing name that starts with `_` as the attribute Python's object would have and asks
`is_safe_attribute` with a stand-in object (IHostSandbox::NativeAttribute). What stays different:
- A public name of an engine value (`s.upper`, `d.items`) reaches `is_safe_attribute` only when
  the subclass overrides it (judged with a stand-in object). The immutable form's refusal of the
  methods that change a list or dict is the engine's own (Dialect::ImmutableSandbox), whatever
  `is_safe_attribute` says.
- A dict key that is also an attribute name (`{'__class__': 1}.__class__`) reads the key; Jinja2
  reads the attribute and refuses it.
- `f.__call__` of a macro or global function is the function itself (0214) and a bound method's
  `__self__` is its string; Jinja2 refuses both as private names. Neither hands the template
  anything it did not hold.
- Under `StrictUndefined`, a private name an engine object does not have (`loop._x`) raises
  `SecurityError`; Jinja2 raises `UndefinedError` because Python's object lacks the name.
- An overridden `call`, `getattr` or `getitem` of the environment is refused with `UnsupportedError`
  when a template loads (the checkpoint is `_sandbox_attribute`), not called.

**Proposal.** Let the policy (ISandboxPolicy) see dict keys that shadow an attribute and a bound
method's `__self__`. Decide whether `f.__call__` and `__self__` should be
refused in a sandbox render.

**Done when** the sweep in python/tests/test_sandbox.py has a case for each item above, either
equal to Jinja2's outcome or listed with the reason it stays different.
