---
status: done
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/733
area: parity
touches: [src/template_module.cpp, src/internal_value.cpp, src/cycle_collector.h, src/error_info.cpp, include/jinja2cpp/error_info.h, docs/api-2.0.md, test/template_module_test.cpp]
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

**Fix (parity batch 2, group C).**
1. A template callable (macro, `caller`, built-in) passed to a host callable is lent for the
   duration of that call (`CallableLease` in internal_value.cpp): the host gets a `UserCallable`
   that, called during the call and on its thread, runs the callable in the render that passed
   it, with that render's `CycleCollector` current again (`CycleCollector::Reentry`), so that
   writes to the render's host data go to the render's cells, not to the module call's. Called
   later it throws `TemplateExpired` with `extraParams[0] == "lent"`. A host's own callable that
   was converted to a template callable comes back as itself (`HostCallableOf`).
2. `ExportCopier` wraps a template callable found anywhere in an exported value, or in a call's
   result, as a call in the module (`MakeModuleCall`), as top-level exports are.
3. Decided: value exports stay a snapshot taken when the module was made. A live view would need
   the module's mutex on every read and hand the host mutable state that the module's calls
   change, which the immutability principle (host data immutable, renders independent) rules
   out. Documented in docs/api-2.0.md (lifecycle items 6 and 8).
Tests: `TemplateModuleTest.TemplateCallableArguments`, `TemplateModuleTest.NestedCallables`.
