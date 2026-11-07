---
status: open
priority: low
area: parity
depends: [0354]
touches: [python/src/adapters.cpp, python/src/environment.cpp, python/jinja2cpp/sandbox.py, src/filters.cpp, src/value_methods.cpp]
shares: [python/tests/test_sandbox.py, docs/python.md]
---
# Sandbox parity leftovers of 0354

**Problem.** Found by the verifier of 0354 (Jinja2 3.1.6), all P3; the sandbox is fail-closed in each
case except where noted:
- Under `StrictUndefined`, `selectattr('__class__', 'eq', 1)` and the other filters with an
  `attribute=` test return `[]` where Jinja2 raises `SecurityError`. Same family as 0413, whose list
  this extends.
- `lipsum.__name__`, `lipsum['__name__']` and `lipsum|attr('__name__')` render `generate_lorem_ipsum`
  where Jinja2 gives `''`.
- The `intercepted_binops` / `intercepted_unops` / overridden `call`, `getattr`, `getitem` check runs
  when a template loads, so a template loaded before the attribute was set renders later includes
  without it (fail-open for that template).
- An `unsafe_undefined` override that returns something other than an `Undefined` renders `''` for
  the engine's values. A custom `is_safe_attribute` receives an empty stand-in for the engine's
  containers and `None` as the value for loop, cycler, namespace and macro attributes.
- The groupby tuple's `grouper` and `list` are not asked of a custom `is_safe_attribute`
  (`ListAdapter::MarkAsNamedTuple`).
- `loop.update()` and `loop.popitem()` succeed; Jinja2 gives `UndefinedError`.

**Proposal.** Fix each in its own place (the list above is ordered by how likely a template meets it);
check the load-time rule again at each render of a template loaded earlier, cheaply (compare a version
counter of the environment).

**Done when.** Each case above has a row in `python/tests/test_sandbox.py` that agrees with the oracle,
and the matching sentence in the Sandbox bullet of `docs/python.md` is gone.
