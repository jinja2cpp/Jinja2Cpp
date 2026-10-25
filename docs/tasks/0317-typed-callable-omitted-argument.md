---
status: open
priority: low
area: api
depends: []
touches: [include/jinja2cpp/user_callable.h, test/user_callable_test.cpp]
---
# A typed callable with an omitted optional argument is not called

**Problem.** Found while fixing 0296. `MakeCallable` has two paths. A lambda with `ArgInfo`
descriptors goes through `UCInvoker`: an optional argument the template leaves out reaches the
function default-constructed, so `StringFn()` renders `''` and `IntFn()` renders `0` (rows in
`StringParamConvert` and `IntParamConvert`). A function pointer, or any `ArgInfoT` descriptor,
goes through `InvokeTypedUserCallable`: the omitted argument is an empty `Value`,
`TypedParamUnwrapper` leaves the parameter without a value, and the function is not called at
all, so `StringViewPtrFn()` renders `None` and any side effect is lost. The same signature
behaves differently by how it was registered. Python has no direct oracle (a Python function
with a required parameter raises `TypeError`, one with a default gets the default).

**Proposal.** Decide one contract for both paths: either an omitted optional argument without a
default is passed default-constructed (the lambda path's current behaviour, least surprising for
existing hosts), or a missing argument raises an error for both. Then make the other path follow
it and add `()` rows for the function-pointer callables in `StringViewPointerParamConvert`.

**Done when** a lambda and a function pointer with the same signature render the same for an
omitted argument, with rows for both in `test/user_callable_test.cpp`.
