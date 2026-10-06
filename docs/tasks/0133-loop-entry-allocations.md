---
status: done
priority: medium
area: perf
touches: [src/statements.cpp#RenderLoop, src/render_context.h#EnterScope]
---
# Each entry into a `for` loop allocates its state anew

**Problem.** The perf scout of 2026-10-06 (master 448bb7c) finds `Render/mitsuhiko_table`
(2.04x Python) at 3,028 allocations per render for a 1,000 x 10 table: three per row,
made each time the inner `{% for cell in row %}` starts. `ForStatement::RenderLoop`
(src/statements.cpp) makes a `LoopFrame` with `make_shared`, enters a scope for `loop`
and another for the body, and builds the loop's list adapter; all of it is freed when the
row ends. `RenderLoop`'s own code is 21% of the case's instructions, malloc/free about
10% and `shared_ptr` release 4%. 0087 and 0100 made a loop's *iterations* cheap; entering
a loop is now the cost, and nested loops over user data (tables, chat messages with
content parts) enter the inner loop once per outer item.

**Proposal.** Measure each with `bench/count.py --baseline`:
1. Keep the `LoopFrame` on the stack, or reuse one per statement and nesting level, and
   allocate it only when `loop` escapes the body (a `loop` stored with `set` or passed to
   a macro or callable keeps its `shared_ptr` semantics; the current `ClonesOnCopy`/owner
   contract from 0088 says how).
2. Enter one scope for `loop` and the loop variables rather than two, or let the scope
   storage (0129) keep its nodes between entries.
3. Find the third allocation (the list adapter for a user list, or the scope map's first
   node) and remove or reuse it.

0117 (name slots) would remove scope maps for loop variables altogether; this task is the
cheap step that does not wait for it.

**Done when.** `Render/mitsuhiko_table` makes at most one allocation per row and its
instructions drop by at least 5%; `test/` and the sanitizer configuration pass, including
templates where `loop` escapes the body.

**Done.** The three allocations were the `LoopFrame`, the enumerator of the row's list
(a heap `polymorphic`) and the row's own list adapter, which the outer loop's enumerator
makes for each item and which stays. A finished loop now puts its frame into a
thread-local pool of 16 (`LoopFramePool`, src/statements.cpp) unless the template kept
`loop`; the frame holds the maps of the loop's two scopes with their names and nodes
(values released), the slots of `loop` and the target, and its enumerator, which
`IListAccessorEnumerator::Rebind` points at the next list of the same type. The frame of
the same loop (a process-unique `ForStatement::m_loopId`, not its address) is on top of the
pool when an inner loop is entered again; another loop's frame drops the names first.
`RenderContext::EnterScope(InternalValueMap&&)` and `ExitScope(InternalValueMap&)` move
the maps in and out of the scope stack.

Against master 8bd9d1f (`bench/count.py --baseline`): `Render/mitsuhiko_table` 3,028 ->
1,026 allocations and -6.54% instructions, `mitsuhiko_table_wide` -6.26%, `config_file`
-3.2%, every other case within -1.8%..+0.1%. Keeping frame entry and exit out of line
(`JINJA2CPP_NOINLINE_INLINE`) matters: inlined, they pushed `InternalValue` assignment out
of the iteration and cost `dict_ops` +2.2%.
