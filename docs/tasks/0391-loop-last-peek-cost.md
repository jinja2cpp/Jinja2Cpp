---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/780
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
`listSize` for a list the template can change: mutable lists (`for_append_in_body`) need the
real peek.

**Done when.** `Render/for_loop_vars` is within +0.1% of 7ba7fbf by `bench/count.py`, or the
perf track accepts the remaining cost.

**Done (2026-11-05).** Making the peek alone cheaper does not get there: dropping NOINLINE
from `PeekNow` gains -0.10% (GCC keeps it out of line), and forcing its unfiltered fast path
inline in `Peek()` costs +1.07%, because `LoopAccessor::GetLoopAttr` then saves six registers
instead of three on every attribute read. Telling `last` from the list's size was tried and
dropped: a list of the render's data becomes mutable on the template's first write, which may
come after the read (`l.pop(0)` after `loop.last` must see the item peeked before it).

What costs is `GetLoopAttr` itself: on master it set up a 0xc0-byte frame with a stack
canary on every call, for the callables the `cycle` and `loop()` cases make, and moved each
result from a temporary through one shared tail. It now writes `index`, `index0`, `first`,
`last` (after the peek), `depth`, `depth0` and a known `length` straight into the value
(`InternalValue::SetImmediate`) and sends the rest to the out-of-line `GetOtherLoopAttr`, so
the common reads run without a frame. Semantics are unchanged: `last` still peeks.

`Render/for_loop_vars` 180,531 Ir against 194,390 on master f015baf (-7.13%) and 192,484 on
7ba7fbf before #755 (-6.2%); `strings` -0.54%, `hf_qwen3` -0.35%, no row up (local
`bench/count.py`). Cases pinning the peek: `statements.for_last_*` (17), of which
`for_last_then_append`, `for_last_then_remove`, `for_pop_context_list_last` and the
`for_last_ctx_*` rows fail if `last` is told by the size.
