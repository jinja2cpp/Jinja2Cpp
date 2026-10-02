---
status: open
priority: high
area: standards
depends: []
touches: [CMakeLists.txt, thirdparty/CMakeLists.txt, .github/workflows/, src/template_env.cpp, src/template_parser.h, src/template_parser.cpp, src/expression_parser.cpp, src/template_impl.h, README.md]
---
# Drop C++14: C++17 floor, C++23 supported

**Problem.** Ruslan decided on 2026-10-02 that 2.0.0 drops C++14: C++17 is the floor and
C++17, C++20 and C++23 are supported (survey in 0008, design in `docs/api-2.0.md`,
decision 4). The library does not even compile at C++23 today: 41 errors, almost all from `nonstd::get_unexpected()`, an expected-lite extension
that `std::expected` lacks (expected-lite selects `std::expected` at C++23; found by the
clang-tidy thread, 0054). The 0054 cleanup batches wait for this change, and so does every
2.0 API task.

**Proposal.**
1. Replace the 40 `get_unexpected()` calls with `nonstd::make_unexpected(x.error())`, which
   compiles with both expected-lite and `std::expected`; build at C++17 and C++23 to prove it.
2. `JINJA2CPP_CXX_STANDARD` accepts 17, 20 and 23 (default 17) and rejects 14;
   `target_compile_features(jinja2cpp PUBLIC cxx_std_17)` so consumers get at least that.
3. CI: rebuild the pairwise matrix (comment at the top of `linux-build.yml`) on C++17,
   C++20 and C++23, keeping its pairwise property; C++23 rows on the newest GCC and Clang,
   and MSVC with `/std:c++latest` (CMake's mapping of `CXX_STANDARD 23`) until a stable
   `/std:c++23` ships. The clang-tidy job runs at C++17, the floor.
4. Drop the googletest 1.16 fallback in `thirdparty/CMakeLists.txt` (1.17+ needs C++17).
5. README: supported standards and toolchains from 0008.

Replacing the `nonstd::` spellings with `std::` is task 0071, not this one, so this PR
stays small enough to land before the tidy batches.

**Done when.** C++17, C++20 and C++23 rows are green, no C++14 row remains, and 0007 and
0008 are closed with a link to this task's PR.
