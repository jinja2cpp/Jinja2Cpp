---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/917
priority: low
area: perf
depends: []
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/host_call_context.h, src/host_call_context.cpp, src/internal_value.h, src/internal_value.cpp, src/template_module.cpp, test/user_callable_test.cpp]
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

## Result (2026-11-16)

Callgrind of `Module/keep` (local Release, 20 iterations, `JINJA2CPP_NAME_HASH_KEY` pinned as CI),
instructions per call at each step:

| Commit | Ir/call | Allocs | Bytes |
|---|---:|---:|---:|
| a64c314 (#673, the 10-28 anchor) | 6,811 | 8 | 368 |
| c299890 (#800) | 7,003 | 7 | 536 |
| eeb221c (#813) | 7,046 | 7 | 552 |
| d908607 (#849) | 7,083 | 7 | 552 |
| 1f489e5 (#884) | 7,182 | 7 | 568 |
| 9541b58 (master) | 7,110 | 7 | 568 |
| this change | 6,863 | 6 | 520 |

Where the creep came from, by inclusive cost against a64c314 (function by function; much of it
moved between frames as inlining changed):
- The 0335 pin path (#695, decision D8): the call's export view in one block with its
  `CallExports` (`ExportCallFirst`, about 270 Ir) and `PinExports` at the call's end (about 210 Ir).
  0348's -1 allocation (#699) hid it when it landed; every step after added to it.
- #709 (0338 D, failure-atomic pins), about +48.
- #813 moved `MakeContextMap` out of internal_value.cpp: the same allocation and dispose, about
  200 Ir a call, out of line.
- #849 (0459) and #884 (0480/0482): `UserCallableParams` destruction out of line, `CallExports`
  keeps the module's `DroppedExports` (a weak_ptr copy, an expiry check and two weak releases a
  call, about 60), and `CallParams`' vector freed out of line.

What this change makes cheaper (Module/keep -3.47%, -1 allocation; keep_written -0.27%):
- The host-call context map (`UserCallableParams::context`) is used again by the next call on the
  thread when the host kept no copy (`IsSoleOwner`, as 0339 asks); a kept copy still expires and
  the next call makes a new map. -165 Ir with the next item, and the allocation.
- `TemplateModuleState::Call` reserves its positional arguments instead of growing the vector.
- `NoteExport`, `AddCell` and `AdoptExports` are inline (once per call, from `ExportCallFirst`): -59.
- `BorrowedCell::IsUnheld` checks the owner's count before the accessor's two virtual calls, so a
  cell of data the module's names hold stops there (`DropUnheld` runs at every call's end): -23.

Remaining: +0.76% over a64c314 locally (+52 Ir), which is the pin path (0335, D8) and the
0480 dropped-exports link, each a fix the cost paid for. Further cuts would need the host-facing
`UserCallableParams` to allocate less (`extraPosArgs`/`extraKwArgs` each allocate an empty
container's `indirect`, `args` a list node), a public-API choice, or an inline buffer for
`CallParams::posParams`, which every macro and callable call shares and would need its own
measurement across the Render rows.
