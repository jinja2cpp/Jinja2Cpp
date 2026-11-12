---
status: done
priority: high
area: robustness
depends: [0252]
touches: [test/allocation_failure_test.cpp]
---
# Survive allocation failure in Load and Render: remaining coverage

**Release impact (0221).** G8 robustness, and the memory contract (docs/api-2.0.md 3.9,
0250's 2.1) promises that `std::bad_alloc` propagates cleanly.

**Problem.** Failing the n-th allocation on master `48f6983` crashed `Load` at 4 of 24 points
and `RenderAsString` at 8 of 96 (probe in
[design/0252-memory-contract-plan.md](../design/0252-memory-contract-plan.md), fact 2): the
`NodeArena::Allocate` ordering, robin_hood tables destroyed after a failed insert or rehash, and
catch handlers that allocate.

**Already done by PR #597 (0253).** The arena ordering fix, robin_hood made exception safe as a
documented local change, the catch-handler audit, and `jinja2cpp_alloc_tests`
(test/allocation_failure_test.cpp), which fails each allocation N in Load, Render and
GetMetadata. Do not redo them.

**Left for this task** (extend test/allocation_failure_test.cpp after #597 merges):
- an "exhausted" mode: every allocation after N fails, not just the N-th;
- i18n (`{% trans %}`, gettext globals) and wide (`TemplateW`) rows;
- an LSan leak check in the sanitizer jobs (no leak after any failure point);
- the arena's `length_error` row: `ResourceLimitExceeded` naming the arena limit.

The successor of robin_hood (0231) must keep passing these tests. Known limit, not fixed here:
nlohmann::json's destructor allocates, so OOM there terminates the process (0279).

**Perf.** Tests only; no library change expected.

**Landed (PR to be linked).** `JINJA2CPP_ALLOC_EXHAUSTED=1` and the ctest entry
`jinja2cpp_alloc_tests_exhausted` (every allocation after the N-th fails); `TemplateW` Load and
Render sweeps (`Wide*SucceedsOrThrowsBadAlloc`) over every row; i18n rows (`{% trans %}` with a
variable and a plural, the gettext globals); a per-test LeakSanitizer check
(`__lsan_do_recoverable_leak_check`) in the ASan jobs; `NodeArenaTest.ArenaLimitIsResourceLimitExceeded`
(the 4 GiB arena cap cannot be reached by a test template, so the row drives `NodeArena` directly
and maps the error as Load does). The exhausted mode found 0485 and 0486, pinned in the tests.
