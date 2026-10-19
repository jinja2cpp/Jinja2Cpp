---
status: done
priority: high
area: api
depends: [0172, 0178]
touches: [include/jinja2cpp/big_int.h, include/jinja2cpp/value.h, include/jinja2cpp/reflected_value.h, include/jinja2cpp/binding/, src/value.cpp, src/internal_value.cpp#Value2IntValue, test/basic_tests.cpp, test/big_int_test.cpp, test/binding/]
shares: [docs/api-2.0.md]
---
# Public `BigInt` value (0178 phase 1, G1)

**Problem.** `Value` has no way to hold an integer beyond int64: since 0172 (#573),
`Value(uint64_t)` above `INT64_MAX` throws. The layout of `Value` freezes at G1, so the big-integer
alternative must exist before the freeze even though arithmetic on it comes later (0270-0272).

**Proposal** (plan sections 3.1 and 9). Add `jinja2::BigInt` (canonical decimal text, `Parse`
returning `std::optional`, comparisons) in `include/jinja2cpp/big_int.h`, append it as the last
`Value` alternative (`sizeof(Value)` unchanged), with `IsBigInt`/`AsBigInt` accessors. `Value(uint64)`,
`Reflect` and the three JSON bindings store a `BigInt` instead of throwing. Until 0270, a BigInt
that enters a template raises OverflowError. Coordinate with 0250's phase 0253, which also edits
`detail::ToStoredInt`: whichever lands second rebases.

**Done when.** The test rows of plan section 9 for 0269 pass (they replace #573's `EXPECT_THROW`
rows), abidiff shows `sizeof(Value)` unchanged, and docs/api-2.0.md documents `BigInt`.

Plan: [docs/design/0178-big-integer-plan.md](../design/0178-big-integer-plan.md).

Done in #596.
