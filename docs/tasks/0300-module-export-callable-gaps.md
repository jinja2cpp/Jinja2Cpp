---
status: open
priority: low
area: parity
touches: [src/template_module.cpp, src/internal_value.cpp]
---
# Callables that do not cross a `Template::MakeModule` export

**Problem.** `Template::MakeModule` (0284) turns each top-level macro or callable of the
module into a `UserCallable` that runs in the module. Three gaps remain where Python Jinja2
passes the object through:

1. A template callable given to an export as an argument, for example a macro or `caller`
   passed to it inside another render (`{% call mod.m() %}...{% endcall %}` with `mod.m` an
   export), reaches the wrapper as an empty `Value`: `PrepareUserCallableParams` converts a
   `Callable` with `IntValue2Value`, which has no host form for it.
2. A macro nested in a container (`{% set d = {'m': m} %}`) is exported as an empty `Value`
   inside the copied dict: only top-level callables are wrapped.
3. A value is exported as a copy made with the module, so a namespace a later macro call
   changes is not seen through `exports` (Python's `module.ns` is the live object). The
   calls themselves share the module's state, as in Python.

**Proposal.** For 1 and 2, give the host form of a template callable a wrapper like the
export's, holding the module or render that owns it with the 0038 weak rule (a call after
the owner ended fails with TemplateExpired). For 3, a live view would need the module's
mutex on every read: decide whether the snapshot is enough.

**Done when** 1 and 2 round-trip with tests, and 3 is decided and documented.
