---
status: open
priority: medium
area: ci
depends: []
touches: [.github/workflows/linux-build.yml, CMakeLists.txt]
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
