---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/708
priority: low
area: robustness
depends: [0331]
touches: [src/internal_value.cpp, src/shared_owner.h, src/template.cpp, src/template_env.cpp, test/borrowed_cells_test.cpp, docs/design/borrowed-cells-memory-order.md]
---
# A uniqueness test on a `shared_ptr` count has no acquire

**Problem.** Some places decide "nobody else holds this storage" from `std::shared_ptr::use_count()`
and then read or write the storage in place:

- `BySharedMutable::IsUnique` (`internal_value.cpp:1183`) and the list adapter's
  `OwnsMutableItemsAlone` (`:2325`), with the `Traverse`/`Clear` of the same adapters (`:2339`,
  `:2349`, `:2922`, `:2925`, `:2939`) that run in the cycle collector.
- `TraverseBoxed` (`:3904`, `:3914`): `m_callable.use_count() == 1` and
  `m_attributes.use_count() == 1` decide whether the collector owns the callable's captures and
  attributes alone.
- `LoadsInPlace` (`template.cpp:86`): `impl.use_count() == 1`, after which the template parses into
  the existing implementation; a render thread that just dropped its copy has not been ordered before
  that write.
- `ChangeGlobals` (`template_env.cpp:485`) writes the shared globals map in place when no render
  holds it.

The module-call path is no longer one of them. Since #695 the kept value holds the cell's
`ValueObject`, and `BorrowedCell::OwnsItemsAlone` (`borrowed_cells.cpp:187`) tests
`ValueObject::use_count()` (an acquire load, R1 in the design document) before it asks the adapter,
so the host's last read happens before the module's next in-place write. Probe A in the document
(section 9), with the handoff after the call returns as the host contract requires, is clean under
TSan.

`use_count()` is a relaxed load in libstdc++ (`bits/shared_ptr_base.h:230`: "No memory barrier is used
here so there is no synchronization with other threads") and in libc++. The drop of the last other
copy is a release, but a relaxed load that reads it does not synchronize with it. The other holder's
reads are therefore not ordered before the write. That is a data race in the C++ memory model on
every platform, whatever the hardware does; on a weak-memory CPU it can also be a wrong value. The
remaining sites need a thread that holds the storage and drops it concurrently with the collector, a
load or a globals change, which the host contract allows (dropping a reference is supported at any
time), but no test reaches them today.

Task 0343 (`scripts/use_count_check.py`, PR #694) lists these calls and fails a PR that adds one;
this task is done when its whole-tree run lists none, or each remaining call carries a
`// use_count: <reason>` comment.

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
- A TSan row exists for each remaining site that a host thread can reach (a kept value dropped while
  the collector runs; a template reloaded while a render drops its copy), and fails before the fix.
- `*BorrowedCells*:*Module*` pass under ASan+UBSan and TSan.
- `Module/keep` and `Module/keep_written` do not move in `bench/count.py`.
- The design document's table (section 12) loses its "gap 1" rows.
- `scripts/use_count_check.py` (0343) run on the whole tree lists no call.

**Resolution.** Two mechanisms, chosen by who owns the pointer:

- The list and dict storage the template owns (`BySharedMutable`, now also under the list
  adapter) is a `ValueObject` holding the container, as recommended: `IsUnique` and therefore
  `OwnsMutableItemsAlone`, `Traverse` and `Clear` read an acquire count. Same allocation count,
  and the pointer is one word instead of two.
- The other sites hold a `std::shared_ptr` that the public API or the environment owns (the
  template's implementation, the globals map, a callable's captures and attributes), so they
  call `IsSoleOwner` (`src/shared_owner.h`): after `use_count() == 1` it copies the pointer and
  drops the copy, an `acq_rel` decrement that reads from the release sequence of the last other
  owner's drop. Unlike the fence of the fallback, TSan models it. `ChangeGlobals` uses it too
  instead of the proposed flag: the thread's globals snapshot keeps the map across renders, so a
  flag would copy on nearly every change, and the drop costs two atomic operations once per
  change.

Evidence: `SharedOwnerTest.CollectorClearsAListTheHostDroppedOnAnotherThread`,
`FirstLoadAfterACopyIsDroppedOnAnotherThread` and `AddGlobalAfterARenderOnAnotherThreadLetGoOfTheMap`
report a TSan race in 3 of 3 runs without the fix and none with it; `TraverseBoxed` has no row
(the collector only reads there, so a race needs a later `Clear` of what it reached, which its
own counts already order). `scripts/use_count_check.py` lists 6 calls on the parent and none
after. The design document's R3 has the argument.
