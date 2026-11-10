---
status: open
priority: medium
area: robustness
depends: [0284]
touches: [src/template_module.cpp, include/jinja2cpp/value.h, test/template_module_test.cpp]
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

**Resolution.** Two recursions. The module's copy of a host argument, param or environment
global now goes through `OwnedCopy` (src/template_module.cpp), which checks
`StackNearlyExhausted()` once per list or dict level and throws `RecursionLimitError`:
`BadResultAccess` with `ErrorCode::RecursionLimitExceeded` from a macro call, an unexpected
result from `MakeModule` (the params are now converted inside its try block). And
`Value::~Value` (include/jinja2cpp/value.h) recursed once per level, so even the host could not
destroy such a value. A scalar now pays one compare of the variant index; a list or dict counts
the containers being destroyed on the thread and recurses as before up to 64 levels, past which
it destroys the rest one container at a time through a worklist. Should that worklist fail to
allocate, the rest is destroyed recursively, as before, so the destructor never throws or
terminates. `Value`'s copy constructor still recurses: a host that copies such a value itself
overflows, as with any recursive structure in C++. `TemplateModuleTest.DeepArgumentFails`
passes lists and dicts 200000 deep as arguments, params and a global.
