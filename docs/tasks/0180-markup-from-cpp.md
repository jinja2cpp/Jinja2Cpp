---
status: open
priority: low
area: api
depends: [0072]
touches: [include/jinja2cpp/value.h, src/internal_value.cpp#IntValue2Value]
shares: [src/value_visitors.h, include/jinja2cpp/user_callable.h]
---
# Markup from C++

**Problem.** `jinja2::Value` has no Markup flag, so context data and user callables cannot
pass safe HTML: under autoescape a host string is always escaped, and a callable that
returns Markup loses the flag in `IntValue2Value`. Python callers pass `markupsafe.Markup`
objects for this (found in 0025, left over from 0051).

**Proposal.** Needs an architect plan, as it changes the public `Value`: a flag next to the
string alternative (like `InternalValue`'s), or a `jinja2::Markup` string type in the
variant. Fold it into the 2.0 `Value` work (0072) if that lands first.

**Done when.** A host value marked as Markup renders unescaped under autoescape and passes
`is escaped`, and a user callable can return one; unit tests in `test/user_callable_test.cpp`.
The parity corpus cannot pin it: its context comes from JSON.
