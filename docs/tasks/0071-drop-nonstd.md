---
status: done
priority: high
area: standards
depends: [0070]
touches: [include/, src/, test/, thirdparty/, CMakeLists.txt, cmake/public/]
---
# Replace optional-lite, variant-lite and string-view-lite with `std::`; pin expected-lite

**Problem.** With the C++17 floor (0070), optional-lite, variant-lite and string-view-lite
only alias the `std::` types (except variant-lite on MSVC, forced to its own variant by
`CMakeLists.txt`), but the code still spells them `nonstd::` (444 uses in 48 files, with
`expected`) and the build fetches and exports all four. expected-lite stays: `std::expected`
needs C++23. Unpinned, it resolves to `std::expected` for a C++23 consumer of a C++17-built
library: the mismatch of 0068.

**Proposal.** Mechanical replacement of `nonstd::optional`, `variant`, `string_view` and
friends by `std::` (the includes, the `optional_CPP17_OR_GREATER` style branches, the MSVC
variant override), then remove those three dependencies from `thirdparty/` (internal,
external and conan modes), `jinja2cpp-config-deps*.cmake.in` and `conanfile.txt`. Keep
expected-lite. The pin `nsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD` landed with 0070
as a `PUBLIC` definition of the library target and in the installed config (Clang 18 with
libstdc++ at C++23 needed it: `<expected>` exists there but declares nothing), so
`Result<T>` is `nonstd::expected<T, ErrorInfo>` for every consumer at every standard. 0070
checked by hand that a Clang 18 C++23 consumer links against the installed GCC C++17
library; turning that into a CI job is 0068.

It touches nearly every file, so it runs alone, in a quiet window right after 0070 and
before the 2.0 API tasks (0072-0075) and the 0054 tidy batches, which then start from the
`std::` spelling.

**Done when.** The only `nonstd::` left is `expected` (and `make_unexpected`), the three
dependencies are gone from every deps mode, and CI is green on C++17/20/23.

**Outcome.** Done in PR #PRNUM. Every `nonstd::optional`, `variant`, `string_view` (and
`get`, `get_if`, `visit`, `nullopt`, `make_optional`, ...) is now `std::`; the dead
`optional_CPP17_OR_GREATER` branch in `user_callable.h` and the `#if 0` `nonstd::value_ptr`
block in `internal_value.h` are gone, and so is the MSVC `variant_CONFIG_SELECT_VARIANT`
override (MSVC now uses `std::variant` like every other compiler). optional-lite,
variant-lite and string-view-lite are removed from internal, external and conan deps
modes, `conanfile.txt`, the installed external config and the README. `nonstd::` is
left only for `expected`, `make_unexpected` (behind `jinja2::MakeUnexpected`) and
`unexpected_type`.
