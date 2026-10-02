---
status: open
priority: high
area: standards
depends: [0070]
touches: [include/, src/, test/, thirdparty/, CMakeLists.txt, cmake/public/]
---
# Replace the nonstd libraries with the standard types

**Problem.** With the C++23 floor (0070), `optional-lite`, `variant-lite`, `string-view-lite`
and `expected-lite` only alias the `std::` types, but the code still spells them `nonstd::`
(444 uses in 48 files) and the build still fetches and exports four dependencies. The 2.0
API (`docs/api-2.0.md`) names `std::optional`, `std::variant`, `std::string_view` and
`std::expected` directly, which also ends the library/consumer mismatch of 0068.

**Proposal.** Mechanical replacement (`nonstd::` to `std::`, the includes, the
`optional_CPP17_OR_GREATER` style branches), then remove the four dependencies from
`thirdparty/` (internal, external and conan modes), `jinja2cpp-config-deps*.cmake.in` and
`conanfile.txt`. `Result<T>` becomes `std::expected<T, ErrorInfo>`.

It touches nearly every file, so it runs alone, in a quiet window right after 0070 and
before the 2.0 API tasks (0072-0075) and the 0054 tidy batches, which then start from the
`std::` spelling.

**Done when.** `grep -rn nonstd include src test` finds nothing but the vendored
`polymorphic` wrapper's history, the dependencies are gone from every deps mode, and CI is
green.
