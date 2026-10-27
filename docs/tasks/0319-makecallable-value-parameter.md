---
status: in-progress
priority: medium
area: api
touches: [include/jinja2cpp/user_callable.h, test/user_callable_test.cpp]
---
# MakeCallable with a Value parameter does not compile

**Problem.** Found by the 0223 plan while probing (rechecked on 3986280). A callable that takes
the argument as a `jinja2::Value` fails to compile with 19 errors: `ArgPromoter` hits `Value`'s
converting constructor.

```c++
auto f = jinja2::MakeCallable([](const jinja2::Value& v) { return v; }, jinja2::ArgInfo{ "v" });
auto g = jinja2::MakeCallable([](jinja2::Value v) { return v; }, jinja2::ArgInfo{ "v" });
```

A host that wants the raw argument has to write `UserCallable{ fn, argsInfo }` by hand and read
`params["v"]`.

**Proposal.** A `Value`, `const Value&` or `Value&&` parameter receives the argument unchanged
(no promotion), in `MakeCallable` and in its overloads with `ArgInfo`. Check the interaction with
0312 (per-argument dispatch, from the 0039 plan) and 0296, which also touch `user_callable.h`.

**Done when.** Both forms above compile and receive the argument as passed, undefined and none
included, for narrow and wide strings; rows in `test/user_callable_test.cpp`.

**Resolution** (with 0334, same PR). `TypedUnwrapParam` hands a `Value` parameter (by value, `const&` or
`&&`) the argument itself through `ValueParam`, without promotion, on every path: lambdas with
`ArgInfo`, function and member pointers, and `ArgInfoT<Value>`. Rows: `PerArgumentDispatch`
(`cref`, `byValue`, `rref`, `echo`, `valueAndText`), narrow and wide.
