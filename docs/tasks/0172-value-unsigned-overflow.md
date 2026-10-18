---
status: done
priority: high
area: robustness
depends: [0067]
touches: [include/jinja2cpp/value.h#Value-constructors, include/jinja2cpp/reflected_value.h, test/basic_tests.cpp]
---
# Unsigned values above `INT64_MAX` wrap to negative

**Release impact (0221).** This is silent corruption of host-provided data and a
decision about the public value model being frozen for 2.0. Resolve it independently
of 0178's full arbitrary-precision integer proposal. A supported failure contract
is preferable to silently wrapping or introducing undocumented precision loss.

**Problem.** Since 0067, `Value` and `Reflect` store every integral type as `int64_t` with
`static_cast`. A `uint64_t`/`size_t` above `INT64_MAX` wraps: `Value(ULLONG_MAX)` renders
`-1`, where Python Jinja2 (arbitrary-precision `int`) prints `18446744073709551615`. Before
0067 such values stored `bool` or did not compile, so this is not a regression, but it is
the same silent-wrong-value class.

**Options.** Store out-of-range unsigned values as `double` (loses precision above 2^53),
throw from the constructor, or add a `uint64_t` alternative to the value model (cost across
the evaluator; would belong with 0072/0140). Needs a decision before implementing.

**Resolution** (PR #573). Fail loudly, as Ruslan decided (D1 in docs/release-2.0.md): `Value`'s integral
constructor and the integral `Reflector` go through `detail::ToStoredInt`, which throws
`std::overflow_error` for a value outside the `int64_t` range. The constructor stays
`noexcept` for every type that always fits (`noexcept(!detail::CanExceedInt64<I>)`). The
boost, nlohmann and rapid JSON bindings had the same wrap for unsigned numbers above
`INT64_MAX` and now throw too; reflected values are created lazily, so the render fails
with `ErrorCode::UnexpectedException`. Exact big integers remain 0178, which can replace
the throw with a big-int alternative.

**Done when.** `Value(ULLONG_MAX)` and `Reflect(uint64_t(-1))` either render the Python
value or fail loudly, with a test row in `test/basic_tests.cpp`.
