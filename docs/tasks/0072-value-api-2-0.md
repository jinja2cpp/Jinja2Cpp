---
status: open
priority: high
area: release
depends: [0071, 0067, 0043]
touches: [include/jinja2cpp/value.h, include/jinja2cpp/string_helpers.h, src/value.cpp, src/value_visitors.h#ValueRenderer, test/basic_tests.cpp]
shares: [src/, test/]
---
# 2.0 API: `Value` accessors and `ToString`

**Problem.** `Value` is the outlier of the public API: Qt-style `isString`/`asString`,
`get`/`getPtr`/`data`, a throwing member and a lenient free function under the same word
(`v.asString()` vs `jinja2::AsString(v)`). Design and decisions in `docs/api-2.0.md`
(sections 2, 3.1, 4 and decisions 3, 3b, 3c, 6).

**Proposal.**
- `IsX()`; `AsX()`/`As<T>()` throwing, returning references; `bool AsX(out) const noexcept`
  with `std::string_view` out for strings, values for scalars, pointers for containers;
  `GetIf<T>()`; `GetData()`; `ToGenericList()`/`ToGenericMap()`; `IsBool`, `IsInt`,
  `IsDouble`, `IsCallable` and their accessors; `NoneValue`; `Value(std::nullptr_t)`;
  `Value(char) = delete`; `jinja2::Visit`.
- `v.ToString()`, `v.ToWString()` and free `ToString(v)`/`ToWString(v)` print as `{{ v }}`
  does (`ToString(5) == "5"`), plus `v.ToString(std::string& out)` (appends),
  `v.ToChars(first, last)` and `std::formatter<jinja2::Value>`, all through one output
  sink shared with the renderer.
- The 1.x names stay as `JINJA2CPP_DEPRECATED("jinja2cpp-2: <new name>")` one-line
  forwards (section 5.1), with the cost cap described there.
- `RecWrapper<T>` becomes a vendored `detail::indirect<T>` (from jbcoe/value_types, the
  reference implementation of C++26 `std::indirect`); `polymorphic` stays only for the
  enumerator interfaces (`docs/api-2.0.md` 3.8, decision 4).
- Move `src/` and `test/` to the new names.

0043 (ordered `ValuesMap`) also edits `value.h`; it lands first or this task takes it over.

**Done when.** The new API is in, every deprecated alias is covered by
`test/v1_names_test.cpp` (built with deprecation warnings off), and `src/`/`test/` build
without deprecation warnings.
