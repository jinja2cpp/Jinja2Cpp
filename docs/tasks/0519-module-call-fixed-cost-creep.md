---
status: open
pr:
priority: low
area: perf
depends: []
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/host_call_context.cpp, src/internal_value.cpp]
---
# A module call's fixed cost grew 4.3% in three weeks, one named exception at a time

**Problem.** Found by the 2026-11-16 perf scout (`/mnt/project-files/perf-track/2026-11-16-scout.md`).
`Module/keep` is one host call into a template module's macro, which hands a 1,000-item host list to
a host callable (bench/jinja2cpp_bench.cpp, `ModuleKeep`). On the CI trend (`bench-data`), it went
from 6,836 instructions at a64c314 (#673's merge, 2026-10-28) to 7,127 at 37c378c (#910): +4.26%,
and +1.87% in the last week alone. Allocated bytes went from 368 to 568.

No single step crossed the bar. Every step above +0.4% was a named exception that paid for a fix:
#709 +0.70%, #800 +0.44%, #813 +0.61%, #849 +0.75% (0459, the memory budget) and #884 +0.92%
(0480/0482, export frees and keep-alive). Some steps went the other way: #822 -0.31% and #898
-0.70%. The drift job never fired (see 0520).

The render-side fixed costs were cut back in 0512 and 0513. A module call has its own entry path
(CallExports, the host-call context map, the call's pinned storages), and no task covers it.

Frames that grew, last week's master (8a573d2) against 9541b58, about +158 instructions per call
locally:
- `CallExports::~CallExports` +63 (0480);
- a `std::list` of named values cleared per call +63;
- an `OrderedMap` hashtable +48;
- `ValueRefExpression::FindByName` +47.

Some of these are moves of code between frames (`MakeContextMap` moved to host_call_context.cpp),
so the cause of each needs tracing step by step.

**Proposal.** Profile `Module/keep` at each named-exception merge, and find which per-call work runs
whether or not the call needs it. Make it lazy, the way #882 tested a member bit before reading
TLS, or move it to the first use. Examples: export walks when nothing was exported or dropped, and
a context map built when the callee does not read the context.

**Done when.** `Module/keep` is back within +1% of a64c314's count (about 6,900 instructions), or
the remaining cost is recorded here with its cause and the fix that paid for it.
