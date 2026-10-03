---
status: open
priority: low
area: style
depends: [0065]
touches: [include/jinja2cpp/string_helpers.h, src/template_impl.h, src/template.cpp, src/internal_value.cpp, src/value_methods.cpp, src/template_parser.cpp, src/string_converter_filter.cpp, src/testers.cpp, src/global_functions.cpp, src/value_visitors.h, src/binding/]
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
- ~~**JSON serializers pass `.c_str()`**~~ Done in the first 0083 PR, which found the
  real cut elsewhere: `tojson` has its own writer since 0019, but `ConvertString`
  (`include/jinja2cpp/string_helpers.h`) dropped everything after a NUL, so a wide string
  value `L"p\0q"` rendered `"p"`. The same function also cut multibyte output to the
  wide source's length (`L"ééé"` became `"é"` under a UTF-8 locale) and read a
  `string_view` past its end up to the next NUL. It now converts NUL-free segments from
  a terminated copy with the right buffer sizes. The binding serializers take sized
  strings too, although nothing in the library calls them any more (see 0084), and so do
  the JSON readers in `include/jinja2cpp/binding/` (a boost::json string or a RapidJSON
  key with a NUL was cut on the way in). Member lookup by name in `rapid_json.h` still
  goes through `c_str()`, so a key with a NUL is not found by attribute access.
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
