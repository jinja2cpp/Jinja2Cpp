---
status: open
priority: medium
area: robustness
depends: [0284]
touches: [src/template_module.cpp, include/jinja2cpp/value.h]
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
