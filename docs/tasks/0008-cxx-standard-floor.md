---
status: in-progress
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


## Decision (Ruslan, 2026-10-02): C++23 for 2.0

Ruslan chose to go past the C++17 recommendation to **C++23** for 2.0.0, so the public API
can use `std::expected` (as `Result<T>`) and the other `std::` vocabulary types, and drop the
nonstd libraries from the API altogether (`docs/api-2.0.md`, decision 4).

What C++23 needs from each toolchain (the parts the library would use: the language mode
and `std::expected`; `<format>` and `std::print` only behind feature-test macros):

| Toolchain | `std::expected` | `<format>` | Notes |
|---|---|---|---|
| GCC (libstdc++) | 12 | 13 | `std::print` 14 |
| Clang with libstdc++ | 19 (the first to report `__cpp_concepts` 202002; not checked here) | 18 (*checked* with libstdc++ 13) | *checked:* Clang 18 + libstdc++ 13 (Ubuntu 24.04's defaults) has no `std::expected`: Clang 18 reports `__cpp_concepts` 201907 and libstdc++ gates `<expected>` on 202002 |
| Clang with libc++ | 16 | 17 | |
| Apple Clang | Xcode 15 | Xcode 15.3 | ([Apple](https://developer.apple.com/xcode/cpp)) |
| MSVC | VS 2022 17.3 under `/std:c++latest` | yes | no stable `/std:c++23` yet: MSVC Build Tools 14.51 has `/std:c++23preview`, the stable switch is announced for 14.52 ([MSVC blog](https://devblogs.microsoft.com/cppblog/c23-support-in-msvc-build-tools-14-51/)); STL features under preview switches carry no ABI guarantee |

Against the oldest LTS distributions:

| Distribution | Default GCC / Clang | Works with the default compiler? |
|---|---|---|
| Debian 12 | 12.2 / 14 | GCC yes; Clang no (needs `clang-19` from apt.llvm.org) |
| Debian 13 | 14.2 / 19 | yes, both |
| Ubuntu 22.04 | 11.2 / 14 | no; `gcc-12` is in the archive |
| Ubuntu 24.04 | 13.3 / 18 | GCC yes; Clang no (`clang-19` is in the archive) |
| RHEL 9 | 11.5 | no; `gcc-toolset-12`+ |

Consequences to carry into the 2.0 work:
- CI rows for C++14/17/20 go; the pairwise matrix (`.github/workflows/linux-build.yml`) is
  rebuilt around C++23 (and C++26 where available), with Clang 19+ and GCC 12+.
- Windows builds use `/std:c++latest` (CMake `CXX_STANDARD 23` maps to it) until 14.52;
  binary packages for MSVC (Conan, vcpkg) should wait for the stable switch or say that
  the ABI follows the MSVC toolset.
- googletest pin: always the current release; the 1.16 fallback goes.
- The README states the toolchain floor.
