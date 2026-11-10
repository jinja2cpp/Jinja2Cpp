---
status: done
priority: medium
area: robustness
depends: [0284]
touches: [src/template_module.cpp, test/template_module_test.cpp]
shares: [test/template_module_test.cpp, python/tests/test_module.py]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/822
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
through `OwnedCopy` (src/template_module.cpp), which throws `RecursionLimitError` at a list or
dict level where the stack is nearly used up, or where what is left of it would not destroy the
copy: `Value`'s destructor recurses through several frames a level where the copy takes one, so
the copy may take at most 1/8 of the stack left below its entry (`DestroyStackFactor`). That
bounds the destruction of a finished copy, and of a partial one unwinding with the siblings it
already copied. The security review of #822 found the destruction crash that a check of the copy
alone left (Debug, 8 MB stack: copies 9000-30000 deep overflowed in their destructor). A macro
call fails with `BadResultAccess` carrying `ErrorCode::RecursionLimitExceeded`, `MakeModule`
returns the error (the params are now converted inside its try block). Scalars convert as before
without a copy, except a `std::string_view`/`std::wstring_view`, which now becomes a string of
the module's own: the module's values outlive the call, and a namespace that stored the view read
freed memory (ASan, found by the same review). `TemplateModuleTest.DeepArgumentFails` (on a 2 MB
thread) passes lists and dicts 200000 deep as arguments, params and a global, and 3000 and 20000
deep, alone and after a deep sibling; `TemplateModuleTest.ViewArgumentIsCopied` pins the views.

`Value`'s own copy and destructor still recurse once per level, so a host cannot copy or destroy
such a value itself, and a module accepts less nesting than Python would; the test destroys its
values with a worklist. An iterative destructor was tried here and dropped: inline in the header
it cost +5.9% instructions on `Module/keep`, out of line in the library (an exported member, a
new symbol `~Value` would need) still +2.2%, against the +0.5% bar. Filed as 0449.
