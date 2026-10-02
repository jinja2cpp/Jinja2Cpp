---
status: open
priority: high
area: standards
depends: []
touches: [CMakeLists.txt, thirdparty/CMakeLists.txt, .github/workflows/, src/template_env.cpp, src/template_parser.h, src/template_parser.cpp, src/expression_parser.cpp, src/template_impl.h, README.md]
---
# Raise the minimum standard to C++23

**Problem.** Ruslan decided on 2026-10-02 that 2.0.0 requires C++23 (survey and toolchain
table in 0008, design in `docs/api-2.0.md`). The library does not even compile at C++23
today: 41 errors, almost all from `nonstd::get_unexpected()`, an expected-lite extension
that `std::expected` lacks (expected-lite selects `std::expected` at C++23; found by the
clang-tidy thread, 0054). The 0054 cleanup batches wait for this change, and so does every
2.0 API task.

**Proposal.**
1. Replace the 40 `get_unexpected()` calls with `nonstd::make_unexpected(x.error())`, which
   compiles with both expected-lite and `std::expected`; build at C++17 and C++23 to prove it.
2. `JINJA2CPP_CXX_STANDARD` defaults to 23 and rejects lower values;
   `target_compile_features(jinja2cpp PUBLIC cxx_std_23)` so consumers get the flag.
3. CI: rebuild the pairwise matrix (comment at the top of `linux-build.yml`) around C++23
   (and C++26 where the compiler has it): GCC 12+, Clang 19+ (Clang 18 cannot use
   libstdc++'s `<expected>`), macOS with Xcode 15+, MSVC with `/std:c++latest` (CMake's
   mapping of `CXX_STANDARD 23`). Switch the clang-tidy job to C++23.
4. Drop the googletest 1.16 fallback in `thirdparty/CMakeLists.txt`.
5. README: the toolchain floor from 0008.

Replacing the `nonstd::` spellings with `std::` is task 0071, not this one, so this PR
stays small enough to land before the tidy batches.

**Done when.** The C++23 CI rows are green, no row below C++23 remains, and 0007 and 0008
are closed with a link to this task's PR.
