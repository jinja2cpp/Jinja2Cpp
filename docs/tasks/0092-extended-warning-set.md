---
status: done
priority: medium
area: build
touches: [CMakeLists.txt, thirdparty/, src/, include/, test/]
---
# Enforce a warning set beyond `-Wall`

**Problem.** `JINJA2CPP_STRICT_WARNINGS` adds only `-Wall -Werror`, and only to the library
target: `jinja2cpp_tests` compiles with no warning flags at all. Under `-Wall` the library
is clean on GCC 13 and clang 18, Release and Debug (checked for 0089, #355). A wider set
finds more, measured 2026-10-03 on master 0cccb8f + #355, whole tree incl. tests,
`-Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnon-virtual-dtor
-Woverloaded-virtual -Wold-style-cast -Wimplicit-fallthrough`, Release, unique sites in
`src/`, `include/`, `test/`:

| Flag | GCC 13 | clang 18 | Where |
|---|---:|---:|---|
| `-Wsign-conversion` | 85 | 48 | expression_evaluator.cpp (46 GCC), lexertk.h (15, vendored), template_parser.h, generic_adapters.h, helpers.h, boost_json.h, tests |
| `-Wunused-parameter` | 0 | 86 | tests only (forloop, statements, basic, macro, filters, ...) |
| `-Wconversion` | 16 | 0 | value_methods.cpp |
| `-Wshadow` | 13 | 1 | testers.cpp (4), value_visitors.h (3), statements.cpp, string_converter_filter.cpp:1171 (both compilers), template_impl.h, template_parser.h, 2 tests |
| `-Wold-style-cast` | 13 | 13 | test/filesystem_handler_test.cpp |
| `-Wredundant-move` | 2 | - | template_parser.cpp:758, :835 |
| `-Wsign-compare` | 1 | 1 | test/filesystem_handler_test.cpp:30 |

`-Wpedantic`, `-Wnon-virtual-dtor`, `-Woverloaded-virtual` and `-Wimplicit-fallthrough`
report nothing in our code.

A second obstacle: dependencies are included with `-I`, not `-isystem` (Boost, fmt,
expected-lite, nlohmann in the internal deps mode), so the same flags report ~540 more
sites inside Boost alone, and `-Werror` would fail on them (`-Wold-style-cast` in
boost/unordered, for example). `thirdparty/thirdparty-internal.cmake` already works around
this per warning (`-Wno-error=maybe-uninitialized` on `boost_variant` INTERFACE, which
also leaks into our own targets and is why 0089 stayed a warning).

**Proposal.**
1. Mark dependency include directories SYSTEM (FetchContent `SYSTEM`, CMake 3.25, or
   `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`) in every deps mode, then drop the per-warning
   `-Wno-error=` workarounds that no longer fire.
2. Apply the strict flags to `jinja2cpp_tests` too.
3. Fix the hits above (shadow and redundant-move are one-liners; sign-conversion in the
   evaluator wants explicit casts or `std::size_t` indices; leave vendored `lexertk.h` and
   `robin_hood.h` to a pragma or a SYSTEM-style include).
4. Add `-Wextra -Wshadow` (and `-Wsign-conversion` if the evaluator fix is clean) to the
   strict set; measure on gcc-12/14 and clang-18/20 in CI before enabling, since newer
   compilers find more. MSVC has its own track (0082).

**Done when.** The strict set includes at least `-Wextra -Wshadow`, covers the tests, and
CI is green on every Linux/macOS compiler with dependencies outside our warnings.

**Resolution (2026-10-05, PR #402).** Re-measured on master 150a5e0: GCC 13 found 93 sites,
clang 18 151 (91 of them `-Wunused-parameter` from the `MULTISTR_TEST` params getter).
- `jinja2cpp_mark_system()` (thirdparty/CMakeLists.txt) copies each FetchContent target's
  interface include directories into `INTERFACE_SYSTEM_INCLUDE_DIRECTORIES`, recursively over
  the dependency's directories; it runs after every `FetchContent_MakeAvailable` of the
  internal mode (Boost, fmt, expected-lite, nlohmann_json, RapidJSON, googletest). The other
  deps modes use imported targets, whose includes are already system.
- The strict set on GCC/Clang is now `-Wall -Wextra -Wpedantic -Wshadow -Wconversion
  -Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast
  -Wimplicit-fallthrough -Werror`, applied to the library and to `jinja2cpp_tests`.
- Fixes: `[[maybe_unused]]` on the getter parameters, explicit casts or `std::size_t`
  indices for sign conversions (vendored `lexertk.h` got five casts rather than a pragma),
  `InternalValue(EmptyValue())` where GCC's `-Wconversion` flagged the implicit
  `EmptyValue` to `InternalValue` choice, renamed shadowing names
  (`TextBlockType::Expression` became `Expr`), `static_cast<bool>` in the filesystem tests.
  `XmlAttr.PerformNegativeTest` dropped its `params`; it now passes them.
- gcc-12 alone reports `-Wredundant-move` on `return std::move(left)` in `StringJoiner`
  (value_visitors.h); both are plain `return left;` now.
- Checked clean with GCC 12/13/14 and clang 18/20 (libc++ too), Release and Debug, C++17/20/23, shared library,
  and boost/nlohmann/rapid bindings. The copied system include directories are wrapped in
  `$<BUILD_INTERFACE:...>` so the dependencies' installed exports do not change. (rapid configured from a git clone with the two patches
  applied, via `FETCHCONTENT_SOURCE_DIR_RAPIDJSON`).
- The `-Wno-error=` workarounds in thirdparty-internal.cmake stay: they cover Boost's own
  build and inlined code GCC 12/14 may still attribute to our files, which this container
  cannot test. Benchmarks and the fuzz harness keep their own flags.
