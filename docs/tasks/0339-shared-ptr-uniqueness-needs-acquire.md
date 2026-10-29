---
status: open
priority: medium
area: robustness
depends: [0331]
touches: [src/internal_value.cpp, src/template_env.cpp, src/template_env_impl.h, test/borrowed_cells_test.cpp]
---
# A uniqueness test on a `shared_ptr` count has no acquire

**Problem.** Three places decide "nobody else holds this storage" from `std::shared_ptr::use_count()`
and then write the storage in place:

- `BorrowedCell::Unshare` asks `OwnsMutableItemsAlone()` (`internal_value.cpp:2000` for lists,
  `BySharedMutable::IsUnique` at `:1183` for dicts). If it is alone, a module call writes the cell's
  items without copying (decision D8 of 0331).
- The same two adapters' `Traverse` and `Clear` (`:2007`, `:2017`, `:2593`, `:2607`) run in the
  cycle collector and empty a container the same way.
- `ChangeGlobals` (`template_env.cpp:485`) writes the shared globals map in place when no render
  holds it.
- `TraverseBoxed` (`internal_value.cpp:3562`, `:3572`): the collector decides from
  `m_callable.use_count() == 1` and `m_attributes.use_count() == 1` whether it owns the callable's
  captures and attributes alone, and then reads (and, through `Clear`, empties) them.
- `LoadsInPlace` (`template.cpp:86`): `impl.use_count() == 1`, after which the template parses into
  the existing implementation in place; a render thread that just dropped its copy of the
  implementation has not been ordered before that write.

Task 0343 (`scripts/use_count_check.py`, PR #694) lists these calls and fails a PR that adds one;
this task is done when its whole-tree run lists none, or each remaining call carries a
`// use_count: <reason>` comment.

`use_count()` is a relaxed load in libstdc++ (`bits/shared_ptr_base.h:230`: "No memory barrier is used
here so there is no synchronization with other threads") and in libc++. The host thread's drop of its
last copy is a release, but a relaxed load that reads it does not synchronize with it. The host's
reads of the kept value are therefore not ordered before the module's next write: a data race in the
C++ memory model, on every platform. On x86 it never shows as a wrong value; on a weak-memory CPU it
can.

TSan reproduces it (`docs/design/borrowed-cells-memory-order.md`, section 9, probe A, kept in the
project files as `0338-memory-order/probe_use_count.cpp`): a module macro appends to `l` and calls
`keep(l)`, a second thread reads each kept value and drops it, and the main thread calls the macro
2000 times. Master gives 1 to 6 reports per run; with the count read by an acquire load (an
experiment on libstdc++'s control block) every report disappears. An `atomic_thread_fence` after the
test is the right production fix but TSan does not model fences, so it cannot confirm it.

**Proposal.** Make the test an acquire that TSan can see.

- Recommended: hold the list and dict storage in a library-owned counted object (a small
  `ValueObject` that owns the `InternalValueList` or `TemplateDict`), so that "alone" is
  `ValueObject::use_count()`, an acquire load, and the whole protocol rests on one count (the R1
  argument in the design document). It also lets `Traverse` and `Clear` use the same test without a
  second kind of count.
- Fallback: a helper `IsSoleOwner(const std::shared_ptr<T>&)` that returns false unless
  `use_count() == 1` and then issues `std::atomic_thread_fence(std::memory_order_acquire)`. Correct
  against a release decrement, but TSan does not model fences, so the TSan row would still report
  and the job would have to skip it.
- `ChangeGlobals`: do not test the count. Set a flag under the shared lock when a snapshot takes the
  map, and have the writer replace the map with a copy when the flag is set; the lock orders the
  flag. The cost is one map copy per change after a render, which is rare.

**Done when.**
- A TSan row built from probe A (a macro that appends and keeps, a thread that reads and drops each
  kept value, 2000 calls) is clean, and fails on master.
- `*BorrowedCells*:*Module*` pass under ASan+UBSan and TSan.
- `Module/keep` and `Module/keep_written` do not move in `bench/count.py` (the count is read once
  per write after an export, not on the hot path).
- The design document's table (section 12) loses its "gap 1" rows.
- `scripts/use_count_check.py` (0343) run on the whole tree lists no call.
