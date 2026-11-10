---
status: done
priority: medium
area: robustness
depends: [0284]
touches: [src/template_module.cpp, test/template_module_test.cpp]
shares: [test/template_module_test.cpp, python/tests/test_module.py]
---
# A deeply nested argument to a module macro overflows the stack in the engine

**Problem.** `TemplateModuleState::Call` copies each argument (`OwnedArgument`, and the `Value`
copy constructor behind it) recursively with no depth or stack check: a list nested 10000 deep
passed to an exported macro crashes the process (segfault, about 14 frames of variant copy per
level). A host can pass such data to `MakeModule(params)` and to a macro call alike. The Python
module stops its own conversion at depth 1000 with `RecursionError`, which hides it for Python
callers only.

**Proposal.** Check `StackNearlyExhausted()` in the copy of `OwnedArgument` and the module's
param conversion (or make `Value` copy iterative), and report the engine's depth error as
`ErrorCode`/`RecursionLimitError` like a render does.

**Done when** a `TemplateModuleTest` row passes a list nested 200000 deep to a macro and to
`MakeModule` and gets an error instead of a crash, under ASan.

**Resolution.** The module's copy of a host argument, param or environment global now goes
through `OwnedCopy` (src/template_module.cpp), which checks `StackNearlyExhausted()` once per
list or dict level and throws `RecursionLimitError`: `BadResultAccess` with
`ErrorCode::RecursionLimitExceeded` from a macro call, an unexpected result from `MakeModule`
(the params are now converted inside its try block). Scalars convert as before, without a copy.
`TemplateModuleTest.DeepArgumentFails` passes lists and dicts 200000 deep as arguments, params
and a global.

`Value`'s own copy and destructor still recurse once per level, so a host cannot copy or destroy
such a value itself; the test destroys its values one level at a time. An iterative destructor
was tried here and dropped: inline in the header it cost +5.9% instructions on `Module/keep`,
and out of line in the library (an exported member, a new symbol `~Value` would need) still
+2.2%, against the +0.5% bar. Filed as 0449.
