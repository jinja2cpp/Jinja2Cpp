---
status: open
priority: low
area: style
depends: [0065]
touches: [src/template_impl.h, src/template.cpp, src/internal_value.cpp, src/value_methods.cpp, src/template_parser.cpp, src/string_converter_filter.cpp, src/testers.cpp, src/global_functions.cpp, src/value_visitors.h, src/binding/]
---
# C++17 idioms clang-tidy does not automate

**Problem.** With C++14 dropped (0070), the `modernize-*` checks are clean, but a manual
pass (Ruslan asked for one, 2026-10-03) finds pre-C++17 code they do not flag. Counted on
the 0065 branch (src/ + include/, vendored files excluded, 32k lines):

- **`boost::optional` in `src/`** (9 sites: `TemplateImpl::Load`/`Render`,
  `ToResult` in `src/template.cpp`, `ListConverter` in `src/internal_value.cpp`). All
  internal; `std::optional` removes the Boost dependency from these files.
- **`.first`/`.second`** (81 uses) where structured bindings name the parts:
  `for (const auto& [name, value] : kwParams)` in `src/value_methods.cpp`, map lookups in
  `src/template_parser.cpp`.
- **`std::string("...")` around literals** (15), for example the defaults in
  `ParseParams` (`src/string_converter_filter.cpp`, `src/global_functions.cpp`) and the
  messages in `src/testers.cpp`: `"..."s`.
- **JSON serializers pass `.c_str()`** where the library takes a sized string
  (`src/binding/boost_json_serializer.cpp:71,81,87`, the nlohmann and RapidJSON ones):
  besides an extra `strlen`, a string with an embedded NUL is cut there. Python's
  `tojson` keeps it (`'a\x00b'|tojson` is `"a\u0000b"`). This is a behaviour fix and
  wants a test and a corpus case.
- **Custom traits read through `::value`** (`IsStringType<L>::value` in
  `src/value_visitors.h`, `IsRecursive<T>::value` in `src/internal_value.h`): add `_v`
  variable templates.

Not worth changing, checked: the seven `return std::move(x)` (the return type differs
from `x`'s, so C++17 does not move implicitly; C++20's P1825 would), the `enable_if`
overload sets in `include/jinja2cpp/user_callable.h` and `src/value_visitors.h` (overload
selection, not branches `if constexpr` could take), `std::tie` comparisons in
`src/template_env.cpp`, and `strtod` on `c_str()` (`std::from_chars` for `double` is
missing from the libc++ that older AppleClang ships).

**Proposal.** One PR per bullet, the JSON one first since it changes output.

**Done when** the bullets are gone or each remaining site is noted as deliberate.
