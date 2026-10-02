---
status: open
priority: medium
area: standards
depends: [7]
---
# Decide the minimum supported C++ standard

**Problem.** The public API must stay C++14-compatible, which keeps the nonstd shims and
a C++14 googletest pin (v1.16.0; v1.17 needs C++17). Every new standard adds a CI row on
top. Keeping the floor costs maintenance; raising it may cost users on old toolchains.

**Proposal.** Find out who still builds as C++14 (issues, package-manager download stats,
downstream projects). If nobody, raise the floor to C++17 in the next major version and
drop the shims where `std::` equivalents exist.

**Done when.** The decision is recorded here and in the README.

## Survey (2026-10-02, for the 2.0 API design in `docs/api-2.0.md`)

Ruslan's criterion: keep C++14 only if C++14 is still well supported and newer standards
are not supported at all somewhere we care about; otherwise follow googletest, which
dropped C++14.

**Default standard of each compiler** (what users get without `-std`):

| Compiler | Default | Since |
|---|---|---|
| GCC | `gnu++17` | GCC 11 (GCC 6-10: `gnu++14`) |
| Clang | `gnu++17` | Clang 16 (Clang 6-15: `gnu++14`) |
| MSVC | `/std:c++14` | still the default in Visual Studio 2022 ([docs](https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-170)) |
| Apple Clang | Xcode projects pass `-std` explicitly | |

Checked in this container: GCC 13.3 and Clang 18.1 both report `__cplusplus == 201703L`
with no flag.

**Oldest supported LTS distributions and their default compilers:**

| Distribution | Support | Default GCC / Clang | Default standard | C++17 complete? |
|---|---|---|---|---|
| Debian 11 bullseye | LTS ended 2026-08-31 (paid ELTS only) | 10.2 / 11 | C++14 | yes |
| Debian 12 bookworm | LTS until 2028-06 | 12.2 / 14 | C++17 / C++14 | yes |
| Ubuntu 20.04 | standard support ended 2025-05 (ESM only) | 9.3 / 10 | C++14 | yes |
| Ubuntu 22.04 | standard support until 2027-04 | 11.2 / 14 | C++17 / C++14 | yes |
| RHEL 8 | maintenance until 2029 | 8.5 (newer via gcc-toolset) | C++14 | yes (`<filesystem>` needs `-lstdc++fs`, not used here) |
| RHEL 9 | until 2032 | 11.5 | C++17 | yes |

Every compiler on every distribution still in support implements C++17 completely, so the
"newer standards not supported at all" half of the criterion does not hold anywhere.
C++14 survives only as a *default*: MSVC (all versions), Clang up to 15 (the default
`clang` on Ubuntu 22.04 and Debian 12) and GCC up to 10 (RHEL 8). For those users a C++17
floor means one flag, and CMake consumers get it automatically from the library target's
`target_compile_features(... PUBLIC cxx_std_17)`.

**What others did.** googletest requires C++17 since 1.17 (`#error C++ versions less than
C++17 are not supported`) and follows Google's
[Foundational C++ Support Policy](https://opensource.google/documentation/policies/cplusplus-support):
drop a standard when all supported compilers default to a newer one *or* ten years after
its release. C++14 (December 2014) passed the ten-year mark in 2024. The current
[support matrix](https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md)
lists C++17 as the minimum, with GCC 11.2, Clang 14, MSVC 2022 and Ubuntu 22.04 / Debian 13 /
RHEL 9 as the floor, and already schedules C++17 itself to end on 2027-12-15. Our own tests
build googletest 1.18 at C++17 and fall back to 1.16 only for the C++14 configuration
(`thirdparty/CMakeLists.txt`).

**Recommendation.** Raise the floor to C++17 in 2.0.0, the one release that breaks the API
anyway. It removes `optional-lite`, `variant-lite` and `string-view-lite` from the public
API (only `expected-lite` stays until C++23), most of task 0068's ODR hazard with them,
the C++14 CI rows and the googletest 1.16 pin. The 1.x line, if one is maintained, stays
C++14. Not yet measured: who builds Jinja2C++ as C++14 today (package-manager stats,
downstream issues); the survey above says the cost to them is one compiler flag.
