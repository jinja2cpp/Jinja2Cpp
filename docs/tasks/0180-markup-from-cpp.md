---
status: in-progress
priority: low
area: api
depends: [0072]
touches: [include/jinja2cpp/value.h, include/jinja2cpp/markup.h, src/internal_value.cpp#IntValue2Value]
shares: [src/value_visitors.h, include/jinja2cpp/user_callable.h, include/jinja2cpp/string_helpers.h, src/error_info.cpp, docs/api-2.0.md, docs/release-2.0.md]
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

## Design (architect plan, 2026-10-18)

`template<class CharT> class BasicMarkup : public std::basic_string<CharT>` in
`include/jinja2cpp/markup.h`, all constructors explicit; `Markup` and `WMarkup` are appended to
`Value::ValueData` as alternatives 13 and 14, so `sizeof(Value)` does not change (a
`static_assert` in `value.h` and a test pin it). Rejected: a flag next to the variant
(`Value` grows to 48 bytes, and the flag is lost on copies of `GetData()`).

- **Is-a rule.** A Markup is a string, as Python's is a `str`: `IsString`, `AsString` (all
  forms), `As<std::string>` and `GetIf<std::string>` accept it; `IsMarkup`, `AsMarkup`,
  `As<Markup>` and `GetIf<Markup>` are exact, and so is `GetData()` with `std::get`.
- **Conversions.** `InputValueConvertor` converts a Markup as its text and sets the
  `InternalValue` flag; `IntValue2Value` makes a Markup of any flagged string, a view of host text
  included. A typed `MakeCallable` parameter receives the base string (`ParamUnwrapper`);
  `UserCallableParams` keep the flag.
- **Equality** on the host side is structural: `Value(Markup("a")) != Value("a")`. Templates keep
  Python's text comparison.
- Operations on a Markup (`+`, `substr`) return plain strings: no Markup arithmetic.

## Next

- A public `markupsafe.escape`-like helper: 0253.
- `MakeCallable` visits every argument combination (15^N instantiations with two more
  alternatives): move to a per-argument visit before the next alternative.
- A host Markup reaches a callable as a copy (a `MarkupView` would be two more alternatives).
- Reading a Markup through the wrong accessor (`GetData()` with `std::get<std::string>`)
  fails to find it rather than dropping the flag silently; a visitor with a generic overload
  receives the Markup as its own type.
