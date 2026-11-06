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
- A public name of an engine value (`s.upper`, `d.items`) never reaches `is_safe_attribute`; a
  subclass that refuses `upper` or `items` is not asked. The immutable form's refusal of the
  methods that change a list or dict is the engine's own (Dialect::ImmutableSandbox), whatever
  `is_safe_attribute` says.
- A dict key that is also an attribute name (`{'__class__': 1}.__class__`) reads the key; Jinja2
  reads the attribute and refuses it.
- `f.__call__` of a macro or global function is the function itself (0214) and a bound method's
  `__self__` is its string; Jinja2 refuses both as private names. Neither hands the template
  anything it did not hold.
- Under `StrictUndefined`, a private name an engine object does not have (`loop._x`) raises
  `SecurityError`; Jinja2 raises `UndefinedError` because Python's object lacks the name.
- An overridden `getattr` / `getitem` of the environment is not called (the checkpoint is
  `_sandbox_attribute`).

**Proposal.** Route a public-name read of an engine value through the same hook when the
environment's `is_safe_attribute` is overridden (the base class's answer is known), and let the
hook see dict keys that shadow an attribute. Decide whether `f.__call__` and `__self__` should be
refused in a sandbox render.

**Done when** the sweep in python/tests/test_sandbox.py has a case for each item above, either
equal to Jinja2's outcome or listed with the reason it stays different.
