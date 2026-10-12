---
status: done
priority: high
area: robustness
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/489
touches: [include/jinja2cpp/template.h, test/binding/nlohmann_json_binding_test.cpp, .github/workflows/linux-build.yml]
shares: [src/internal_value.cpp, src/generic_adapters.h]
---
# Segfault iterating arrays reflected from `nlohmann::json`

**Problem.** Passing an `nlohmann::json` array through `jinja2::Reflect` and iterating it
in a template (`{% for x in l %}`) crashed in
`GenericListAdapter<ByRef>::Enumerator::MoveNext` (`src/internal_value.cpp`, around
line 425). Found by the parity probe driver on 2026-10-01; not root-caused then. A
by-reference adapter over a temporary was the first suspect.

**Root cause.** Not a lifetime bug. Before 2.0 the library was built as C++14, where
optional-lite and variant-lite use their own `nonstd::optional`/`nonstd::variant`; the probe
driver was compiled as C++17, where the same headers alias `std::optional`/`std::variant`.
`IListItemAccessor::CreateEnumerator()` returns `optional<ListEnumeratorPtr>`, so the
header-only nlohmann accessor (driver's layout) and `GenericListAdapter` (library's layout)
read one object with two layouts. Reproduced on f2c05c0 by linking a C++17 test object
against the C++14 library: UBSan reports `load of value 64, which is not a valid value for
type 'bool'` in `nonstd/optional.hpp`, and every loop renders empty. With matching
standards the same test is clean, on f2c05c0 and on master. 2.0 removed the cause (0071
moved the API to `std::optional`/`std::variant`, C++17 floor).

**What remained.** `Result<T>` is still expected-lite's `nonstd::expected`, chosen by the
`nsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD` flag that CMake and pkg-config pass. A
consumer built without it at C++23 (GCC 14, MSVC `/std:c++latest`) gets `std::expected`
under the same name: the same mismatch class.

**Done.** `template.h` now defaults the flag before including expected-lite and fails with
`#error` when `nonstd::expected` already resolved to `std::expected` in that translation unit.
`NlohmannJsonTest.ArrayIteration` covers top-level, nested, filtered and filter-consumed
iteration of reflected arrays, and the clang-18 `address+undefined` CI job now builds with
nlohmann bindings so it runs under the sanitizer.
