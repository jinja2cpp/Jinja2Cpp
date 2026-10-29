---
status: in-progress
priority: medium
area: ci
depends: []
touches: [.github/workflows/linux-build.yml, CMakeLists.txt, test/debug_containers_test.cpp, src/stable_sort_values.h]
---
# A test job with a checking standard library (iterator and reference invalidation)

**Problem.** ASan sees a stale iterator or reference into a `std` container only when the memory
it points to is freed. A use after `erase`, `insert` or `push_back` without reallocation, or an
iterator into a `robin_hood` / `std::vector` that moved its elements, reads valid memory with the
wrong value. That is the dynamic half of the lifetime class (REVIEW.md). Task 0265 rejected
aborting assertions in production builds. In a test job, an abort is a finding.

**Proposal.** A Linux job that builds and runs the tests with libstdc++'s `_GLIBCXX_DEBUG` (safe
iterators; it changes the ABI, so it is a separate build directory with all deps built in the same
mode), or Clang + libc++ with `_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG`. Add
`detect_container_overflow` (libc++ or `_GLIBCXX_SANITIZE_VECTOR`) to the ASan job if it is cheap.
Keep it on the pairwise C++ standard matrix, as one more column (see the comment at the top of
`linux-build.yml`).

**Done when** the job runs the full test suite on PRs that touch `src/`, `include/` or `test/`, and
a seeded use of an invalidated vector iterator in a scratch test fails it.

**Implemented.** `JINJA2CPP_DEBUG_CONTAINERS=libstdc++|libc++` (CMake, global, so dependencies and tests
share the ABI) and the `debug-containers` job in `linux-build.yml`: gcc-14 C++20 with `_GLIBCXX_DEBUG`
and clang-18 C++17 with libc++ hardening `debug`. The libstdc++ row also runs ASan+UBSan. `detect_container_overflow` stays off (open): `_GLIBCXX_SANITIZE_VECTOR`
makes GCC 13's ASan report gtest's own vector growth, and libc++'s annotations report the same, because only
the library is instrumented; fixing that means instrumenting the dependencies too. Ubuntu's libc++ is not
ASan-aware (alloc-dealloc-mismatch in libc++abi's exception strings), so the libc++ row has no sanitizer, and `ModuleCallTest.CallsStayFlat` skips there
(its allocation counter sees that libc++abi allocation as a leak per failing call). `DebugContainersTest.MisuseAborts` seeds an erase-invalidated
iterator (libstdc++) or a hardened out-of-range index (libc++) and expects the abort; a plain build skips it
and the job's last step fails on a skip. libc++ 18 has no iterator-invalidation mode any more (the old
`_LIBCPP_DEBUG` is gone), so that row checks bounds and preconditions only.

**First finding.** `_GLIBCXX_DEBUG` checks that a sort predicate is irreflexive by comparing an element with
itself, which a one-element sort never does. The value sorts (`groupby`, `dictsort`, `sort`, `unique`,
`tojson`/`pprint` key order, list `sort`) threw on a lone `None` or namespace key where Python's `sorted()`
compares nothing. `StableSortValues` (`src/stable_sort_values.h`) skips the call for fewer than two items;
`ExpressionTest.CyclesThroughObjects` and the parity cases `filters.dictsort_single_none_key` and
`filters.tojson_non_string_keys` pin it. Not covered: a comparator that throws on two or more equal
unorderable keys (Python raises there as well).
