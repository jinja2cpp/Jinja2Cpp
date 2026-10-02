---
status: open
priority: high
area: robustness
depends: []
touches: [include/jinja2cpp/value.h#Value-constructors, test/basic_tests.cpp]
---
# `Value` from unsigned and wide integers stores `bool`

**Problem.** `jinja2::Value` has constructors for `int`, `float` and `double`; every other
arithmetic type goes through the generic `Value(T&&)`, which hands the value to the
variant's converting constructor. Which alternative it picks depends on the variant
implementation, so on the C++ standard (found in the 0056 API survey, `docs/api-2.0.md`):

| Expression | C++14 (nonstd variant) | C++17/20 (`std::variant`) |
|---|---|---|
| `Value(5u)`, `Value(5LL)`, `Value(short(5))`, `Value(uint32_t(5))` | `bool` (`true`) | `int64_t` |
| `Value(size_t(5))` | `bool` | does not compile |
| `Value('c')` | `bool` | `int64_t` |
| `Value(nullptr)` | `Value(const char*)`: throws `std::logic_error` | same |

`{{ n }}` with `n = Value(5u)` prints `True` in the default C++14 build. Separately,
`Reflect(5LL)` does not compile on LP64 platforms: `JINJA2_INT_REFLECTOR` covers the
`<cstdint>` types, and `int64_t` is `long` there, so `long long` has no reflector.

**Proposal.**
- A constructor template for every integral type except `bool` and `char` that stores
  `static_cast<int64_t>(val)`, as `Reflect` already does for the `<cstdint>` types.
- `Value(std::nullptr_t)` stores `None` (Jinja2's `None`).
- `Value(char)`: deleted, pending the 0056 decision (number or one-letter string); until then
  storing `int64_t` matches `Reflect(char)`.
- Reflectors for `long long`, `unsigned long long` (and `long`/`unsigned long` where they
  differ from the `<cstdint>` types).
- Tests: one row per type above in a `Value` construction test, compiled in the C++14 and
  C++17 CI rows (both already exist).

**Done when.** The table above reads `int64_t` (and `None` for `nullptr`) in every column,
and `Reflect(5LL)` compiles.

**Next.** This changes what existing code stores (bool to int), so the release notes must
say so; it is a fix, but a visible one.

**Progress.** The `Reflect` half is done in PR #334 (task 0075): `jinja2::Reflector` has
partial specialisations for every integral type except `bool` (stored as `int64_t`) and
every floating-point type, so `Reflect(5LL)`, `Reflect(size_t{})` and `Reflect(1.5L)`
compile; `JINJA2_INT_REFLECTOR` is gone. The `Value` constructors remain.
