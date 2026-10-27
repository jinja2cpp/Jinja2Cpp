---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/683
priority: high
area: robustness
touches: [src/lexer.cpp, src/json_reader.cpp, src/big_int.cpp, thirdparty/internal_deps.cmake, sbom/components.json]
shares: [src/filters.cpp, src/value_methods.cpp, thirdparty/CMakeLists.txt, thirdparty/thirdparty-internal.cmake, thirdparty/thirdparty-external.cmake, thirdparty/thirdparty-conan-build.cmake, conanfile.txt, docs/dependencies.md]
---
# Float parsing follows the C locale

**Problem.** Jinja2C++ reads decimal numbers with `strtod`, which uses the decimal point of
the process's `LC_NUMERIC`. A host that calls `setlocale(LC_ALL, "")` under a decimal-comma
locale (de_DE, fr_FR, ru_RU) gets `1.5` read as `1`: float literals in templates
(`src/lexer.cpp:78`), the `float` filter and the rounding helpers (`src/filters.cpp`, four
calls), big-integer conversion (`src/big_int.cpp:368`). The `{% meta %}` reader works around it
by swapping in `localeconv()->decimal_point` (`src/json_reader.cpp:579`), which races with a
concurrent `setlocale`. 0039 needs an exact, locale-free decimal-to-double conversion too.
`std::from_chars` for `double` is missing from the libc++ of older AppleClang (0083).

**Proposal** (docs/design/0039-float-value-plan.md, 4.5, approved 2026-10-27): vendor
`fast_float` (header-only, MIT/Apache-2.0/BSL-1.0; libstdc++ 12 uses it for `from_chars`)
as an internal dependency, pinned with `URL_HASH` under the immutable-pin rules, listed in
`sbom/components.json` and in the conan and external dependency modes. One helper in `src/`
wraps it; every `strtod` site and the `json_reader.cpp` workaround use the helper. Python's
`float()` rules stay as they are: an exponent out of range gives inf or 0.

**Done when.** No `strtod`/`strtof` call is left in `src/`; a unit test sets `LC_NUMERIC` to a
decimal-comma locale when one is installed (skipped otherwise) and renders `{{ 1.5 }}`,
`{{ '2.5'|float }}` and a `{% meta %}` number correctly; `bench/count.py` shows no lexer
regression.
