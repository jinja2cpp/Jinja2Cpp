---
status: open
priority: low
area: perf
depends: [0304]
touches: [src/statements.cpp#LoopState, src/statements.cpp#LoopAccessor]
---
# `loop.last` peeks out of line since 0304

**Problem.** Since 0304 (#755) a loop fetches its next item after the body, so a body that reads
`loop.last` (or `nextitem`) peeks: `LoopAccessor::GetProperty` calls `LoopState::Peek`, which
calls the out-of-line `PeekNow`. Before, the loop had fetched the item itself. The perf track
approved #755 with this one named cost: `Render/for_loop_vars` +0.84% (+1,610 Ir per render).
Other loop benchmarks got faster (`mitsuhiko_table` -1.0%, `for_range` -1.3%).

**Measured (2026-11-05, master 1607e99, callgrind, 10 renders of for_loop_vars).** Putting the
unfiltered fast path inline in `Peek()` (no `PeekNow` call) gains only -0.16%. The fetch moves
into `GetLoopAttr` (+33.8k Ir; `RenderLoopInSlots` -50.5k), but GCC then calls
`InternalValue::operator=(InternalValue&&)` out of line for `Item(index0 + 1) = GetCurrent()`
(+27.7k Ir), which it inlines in `RenderLoopInSlots`. Mind the 0188 trap: extra code in the
`GetProperty` switch can stop it inlining into `GetLoopAttr`.

**Proposal.** Get the fetch in a peek to cost what it costs in the loop. For example, write the
peeked item without an out-of-line move assignment (an enumerator call that stores into the
slot, or constructing in place), or let the loop prefetch when the parser sees the body read
`last`/`nextitem` and the list cannot change during the body. Do not answer `last` from
`listSize`: mutable lists (`for_append_in_body`) need the real peek.

**Done when.** `Render/for_loop_vars` is within +0.1% of 7ba7fbf by `bench/count.py`, or the
perf track accepts the remaining cost.
