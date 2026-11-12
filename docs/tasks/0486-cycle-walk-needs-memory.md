---
status: open
priority: low
area: robustness
depends: [0256]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, test/allocation_failure_test.cpp]
---
# Reference cycles leak when memory stays spent at the end of a render

**Problem.** Found by the exhausted mode of `jinja2cpp_alloc_tests` (0256) under ASan.
`CycleCollector::Collect` (src/cycle_collector.cpp) runs the walk that breaks a render's
reference cycles (0283) at most twice; each walk allocates its tables (`CollectOnce`,
`WalkAndClear`), and when both throw `std::bad_alloc` the cycles are left unbroken and leak, as
the comment there says. The N-th-only sweep passes, since the second walk gets the memory the
first one freed. With every allocation failing after the N-th, LeakSanitizer reports the cycles
of the rows `{% set ns.self = ns %}`, `cycler(ns)`, `l.append(l)` and
`items.append(items)` through a host alias (`Templates/AllocationFailureTest.RenderSucceedsOrThrowsBadAlloc/6`
and `/7`, narrow and wide). The render still ends with `std::bad_alloc`, and the next render
works; the memory of the failed render's cycles is lost for good.

**Proposal.** Make the end-of-render walk allocation-free, or let it allocate only while notes
are made, where a failure is still a plain `std::bad_alloc`: grow the walk's tables (the
`noted` set, the visit stack, the counts) as candidates are noted, so that `Collect` only uses
storage it already has; or mark visited objects with an intrusive bit in `ValueObject` instead
of a hash set. Measure with `bench/count.py`: the note path runs on every container store.

**Done when.** `Case::makesCycles` in test/allocation_failure_test.cpp is gone (the exhausted
mode leak-checks every row) and `jinja2cpp_alloc_tests_exhausted` passes under ASan.
