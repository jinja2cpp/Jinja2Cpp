---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/681
priority: high
area: api
touches: [include/jinja2cpp/user_callable.h, test/user_callable_test.cpp]
---
# MakeCallable visits every combination of argument types

**Problem.** A lambda registered with untyped descriptors (`MakeCallable(fn, ArgInfo{...}...)`)
goes through one `std::visit` over all N argument variants
(`detail::InvokeUserCallable`), which instantiates `UCInvoker` and its SFINAE probe for every
combination of `Value` alternatives: 16^N today, 17^N once `float` joins (0039). One lambda
with N `const std::string&` parameters, g++ -O2: N = 2 takes 4.6 s and 170 KB of code, N = 3
takes 41 s and 2.4 MB, N = 4 extrapolates to about 10 minutes. Typed descriptors (`ArgInfoT`),
function pointers and member pointers already visit one argument at a time.

**Proposal** (docs/design/0039-float-value-plan.md, 4.4, approved 2026-10-27):
- A callable with a deducible signature (a non-generic lambda or functor with one
  `operator()`, a function pointer, a member pointer) gets its parameter types from a callable
  traits helper, and each argument is visited alone (N x 17 instantiations). One argument
  contract for both paths: the unwrapped alternative through `Promote`, None promotes to
  `T()`, an argument that cannot convert makes the call return None. A `Value`,
  `const Value&` or `Value&&` parameter receives the argument unchanged.
- A generic callable (`auto` parameters, overload sets) keeps the joint visit for up to two
  untyped arguments; from three, a `static_assert` asks for `ArgInfoT<T>` descriptors.
- Same files as 0319 (a `Value` parameter does not compile) and 0320 (the two paths differ on
  an omitted argument): fix them in the same PR, or one after another in the same thread.

**Done when.** The 3-argument probe compiles in under 5 s; `test/user_callable_test.cpp`
passes unchanged; new rows cover a 4-argument untyped lambda, a `const Value&` parameter, a
mismatched argument returning None and a 2-argument generic lambda, narrow and wide.
