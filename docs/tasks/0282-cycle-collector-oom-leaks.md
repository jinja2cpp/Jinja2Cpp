---
status: open
priority: low
area: robustness
depends: [0110, 0256]
touches: [src/cycle_collector.cpp, src/cycle_collector.h, test/allocation_failure_test.cpp]
---
# Reference cycles leak when an allocation fails during a render

**Problem.** Found while adding a cycle row to the allocation-failure sweep of 0253 (PR #597):
`{% set ns = namespace(l=[]) %}{% set ns.self = ns %}{% set l = [1] %}{{ l.append(l) or '' }}`
rendered with the Nth allocation failing passes (result or `bad_alloc`, the template renders
again), but LeakSanitizer reports about 4 KB in 75 allocations, all lists and dicts of the
cycles. Two causes in src/cycle_collector.cpp:
- `Add` inserts the container into `noted` before `candidates.push_back`; when the push
  fails, the container stays marked as noted, every later note of it returns early, and its
  cycle is never collected.
- A store whose `NoteStore` throws has already linked the containers, and `Collect`/`Finish`
  swallow a failed walk by design ("the cycles not broken leak").

**Work.** Undo the `noted` insert when the push fails; note before the store links the
containers where the call sites allow it; give `Collect` a fallback that needs no allocation
(for example emptying every candidate when the walk cannot finish, which is safe only for
containers nothing outside holds, so it needs the reference-count check without the walk's
tables). Then add the cycle row to `jinja2cpp_alloc_tests`; the address+undefined CI job runs
that binary with LeakSanitizer on.

**Done when.** The cycle row passes under ASan/LSan with no leak.
