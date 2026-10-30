# Memory order of the borrowed-cell protocol

Written 2026-10-28 on master `81ce81a`, revised 2026-10-30 on master `42873f9` (after #694 and
#695), step C of the route in
[0338](../tasks/0338-borrowed-cell-protocol-safety-proof.md) (item 5, "memory order by design and
argument, not in TLA+"). It covers `src/borrowed_cells.{h,cpp}`, `src/cycle_collector.{h,cpp}` and
`src/template_module.cpp`, and the state they share with the rest of the library. Line numbers are
those of `42873f9`. It changes no code. What it finds is filed as tasks 0339 to 0342 and 0346.
Task 0339 closed gap 1 since (section 4, R3), and task 0342 gap 3 (section 3, E4).

The claim is narrow. **Given the host contract in section 1, every access the protocol makes to
state that more than one thread can reach is either ordered by a lock, ordered by a
happens-before edge the library creates (an acquire/release pair on a reference count), or reads
state nothing writes any more.** The first version of this document found two places that did
not meet that claim, with TSan reproducing both; #695 closed them. Today:

- the uniqueness tests on `std::shared_ptr` counts had no acquire (task 0339, now closed: the
  template's list and dict storage is a `ValueObject` with an acquire count, and the other sites
  test with `IsSoleOwner`, an `acq_rel` decrement of the same count);
- a global's item link can be dropped on a host thread while the render runs (task 0346, found by
  reading, not reproduced);
- the 32-bit epoch wrapped (task 0342, now closed: the epoch is 64 bits and the link stays at two
  words).

Closed by #695: the foreign unlink of a live cell (task 0340: every link but a global's owns its
cell, so letting go is a count release) and the call-id wrap (task 0341: no code compares a stored
call id with the running one any more). The TLA+ model in `tla/` says nothing about memory order:
it interleaves atomic steps, so it assumes sequential consistency (`tla/README.md`, "Not
modelled").

An analogy for the shape of the argument. The library is a print shop with one proof room per
render. Each room's table (the render's registry of cells) is only ever touched by the person in
that room. Three things cross the walls: a notice board with two numbers on it (the counters, and
anyone may glance at them), finished proofs carried out the door (frozen cells and kept values),
and the shop's reference binders (the module's state and the environment, behind a sign-out
sheet, which is the mutex). Nobody ever edits a proof once it is carried out; when the shop wants
to reuse a proof's paper, someone has to check that the last reader has put it down. That check
was the one that was not rigorous (gap 1, closed by task 0339): glancing at the reading desk is not
enough, the checker has to sign the same sign-out sheet the reader signed when leaving.

## 1. Host contract the argument rests on

These are the host behaviours the protocol supports. A host that breaks one gets a data race the
library does not promise to prevent.

- **H1. Host data is immutable while any render can read it.** Lists and maps passed as params,
  environment globals and values a host callable returns are not written by the host during a
  render, and nothing the library does writes them (decision D2 of 0049). The environment's
  globals are changed through `TemplateEnv`, which copies on change (section 7).
- **H2. A render and its adapters live on one thread.** The adapters of the host's data, the
  render's registry of cells and its collector are used by the thread that runs the render. A
  value handed to a callback is valid during the call and is used on the callback's thread.
- **H3. A kept value is read only after the call that handed it out has completed.** A value that
  a render or a module call returns, or hands to the host with a callback argument, is a read-only
  snapshot (decisions D4 and D8 of 0331), but only once the enclosing render has completed
  (`Finish` and the collector's teardown included) or the exported module invocation has
  returned. Reading earlier on another thread is not supported (`docs/api-2.0.md`, item 9): until
  then a value handed out directly is the render's own storage, which later writes change (`h.append(3);
  keep(h); h.append(4)`, `BorrowedCellsKept.WrittenBeforeAndAfter`), and a queue or thread start
  inside `keep` does not order those writes before the reader. The host may read it on another
  thread after the completion, through a handoff ordered after it (a mutex, a queue, a thread
  start).
- **H3b. Dropping or copying a reference to a kept value is supported at any time, on any
  thread.** That is a reference count operation, not a read of the contents. Since #695 it never
  writes the live cell, because every link but a global's owns its cell (section 8); a global's
  item link is the exception (task 0346).
- **H4. Concurrent renders share only H1 data and kept values.** Two threads rendering with the
  same `Template` or `TemplateEnv` share the parse tree, the environment's callables and
  settings, and the globals map, all read-only during a render (loads and `AddGlobal` take the
  environment's lock).
- **H5. Template module calls take turns.** One module's calls are serialized by its mutex; a
  call nested through a host callback runs on the thread that holds it.
- **H6. During the collector's walk, other threads only copy or drop references they already
  hold.** The walk reads the counts of its nodes one by one, so a thread that makes a reference to
  a *different* object of the walk than the one it holds, and then drops the first, can make every
  count look internal. Three facts keep that out: a `Value` copy shares its accessor and takes no
  new reference to another object; the values this render handed out (`keep`) are unreadable
  until the render, its collector included, has completed (H3), so another thread can copy and drop
  them but not follow their contents; and values of earlier renders enter this render only as the
  host's data, through host adapters that `IsTraversable` excludes (`cycle_collector.cpp:175,210`),
  so they are not in the walk. Found by the cycle-collector spec (task 0338 step E, PR #707): with
  H6 removed, TLC finds a walk that empties a list the host still reaches.

## 2. What is shared across threads

Only five kinds of state are reachable from more than one thread. Everything else is either a
`thread_local`, a stack object of a render, or an object only one thread can name.

| # | State | Written by | Read by | Protection | Section |
|---|---|---|---|---|---|
| S1 | `CellCounters::s_epoch`, `s_live` | every thread that promotes a cell or ends a render that made one | every adapter read; `QuietEpoch` | relaxed atomics, per-thread meaning (section 3) | 3 |
| S2 | reference counts of `ValueObject`; `std::shared_ptr` counts of list and dict storage, `globalValues`, `Impl` | any thread holding a reference | the uniqueness tests | atomic RMW; the uniqueness tests are the weak point | 4 |
| S3 | a module's state: its cells registry, context, callback, `calls` and `convertedParams` | the thread that holds the module's recursive mutex | the same | `TemplateModuleState::mutex` | 5 |
| S4 | frozen cells, kept values and the objects they own | no one after publication | any thread the host passes them to | publication by the host's own synchronization plus the call's return | 6 |
| S5 | the environment: `globalValues`, caches, `globalsGeneration` | writers under `guard` | renders under a shared lock, and one acquire load without it | `shared_timed_mutex` plus one release/acquire atomic | 7 |

What is not shared, and why that matters for the argument:

- **A live cell and its registry.** `BorrowedCells::m_cells`, `BorrowedCell::m_links`,
  `m_exportedIn`, `m_frozen` before the freeze, and every `CellLink` field of an attached adapter
  are reached from one thread: the one that made the render (or, for a module, the one that holds
  its mutex). The registry is found through `CycleCollector::s_current`, a `thread_local`, so a
  thread cannot name another thread's registry. This is the invariant behind most of the
  argument; section 8 states what keeps it true, and the one remainder where it does not (gap 2, the global's own link).
- **The collector.** `CycleCollector` is a stack object, `s_current` is thread-local, and its walk
  runs at the end of the render that owns it.
- **Other `thread_local` state**: `t_pendingDestroy` (the deferred-destroy list, trivially
  destructible, `internal_value.cpp:78`), the render workspace and its globals snapshot
  (`render_workspace.h:44`), `LookupCache` (`render_context.h:184`), the render limits
  (`recursion_guard.h:63-123`), `t_failure` (`render_failure.cpp:154`), and the `markup.h:66`
  buffer. Each is read and written by its own thread only. The one that can hold an object another
  thread made is `t_pendingDestroy`: an object whose last reference a thread drops is deleted by
  that thread, so a value made on thread A may be destroyed by thread B, which is the reason the
  deletion path (`DestroyValueObject`) must not assume the object's maker (section 5).

## 3. The counters (S1)

```
src/borrowed_cells.h:52   Epoch()        s_epoch.load(relaxed)
src/borrowed_cells.h:55   Live()         s_live.load(relaxed)
src/borrowed_cells.h:62   AddLive(n)     s_live.fetch_add(relaxed)
src/borrowed_cells.h:63   RemoveLive(n)  s_live.fetch_sub(relaxed)
src/borrowed_cells.cpp:39 NextEpoch()    s_epoch.fetch_add(relaxed)  (64 bits since 0342; a CAS loop before)
```

The counters carry no data. Nothing is published through them: a reader that sees a value does not
go on to read anything that a writer wrote before writing the counter. They are hints that decide
whether to take a slower path, and the slower path takes its own look at the registry. That is
what makes relaxed enough, and the argument has to show that no wrong hint can lead to a wrong
answer.

**What an adapter does with the epoch.** `CellLink::Get` returns null without a lookup when
`m_seenEpoch == Epoch()` (`m_state` since 0342). `Find` stores the epoch it read *before* it looks the cell up
(`borrowed_cells.cpp:240`), so a promotion after that read moves the epoch past the stored value
and the next read looks again. `ItemAdapters` and `CellStep` store an epoch for a whole block or
enumeration with the same rule.

**E1. A thread never misses its own promotion.** The registry that the thread's adapters consult
is the thread's own, so only that thread's promotions (`Adopt`) and render ends (`Finish`) matter
to them. Take an adapter that stored epoch `v` at time t1, and a promotion by the same thread at
t2 > t1 that moved the counter to `v'`. Both are in `s_epoch`'s modification order, and `v'` is
after `v` in it, because `v'` was made by a read-modify-write that observed `v` or a later value
(`NextEpoch` is a `fetch_add`, which reads the latest value in the modification order; it was a CAS
loop before 0342, with the same property). Coherence (read-read, write-read) says a load that follows the
promotion in program order returns `v'` or a later value. Later values are larger, apart from the
wrap-around (E4), so the load is not `v`, and the adapter looks again. A block destroyed on
another thread (the host drops a kept value) compares with a load that happens after the handoff,
so it sees every promotion made before the handoff; one made after is the remainder of gap 2.

**E2. Other threads' moves cost a lookup and nothing else.** A promotion on another thread moves
the epoch too, so a quiet adapter takes the `Find` path once and finds nothing in its own
registry. That is a performance cost, not a correctness one: the lookup is a read of the thread's
own table. A stale (old) epoch value read because of another thread's promotion can only be a
value the adapter already compares unequal or equal to for E1's reason.

**E3. `Live` is read after the epoch, for the same thread's cells.** `QuietEpoch` (`borrowed_cells.h:443`)
returns the epoch only if `Live() == 0`, so that a block of item adapters made while no cell
exists can skip all its slots until the epoch moves. `Adopt` counts the cell (`AddLive`) before it
moves the epoch (`borrowed_cells.cpp:413-415`), and `Finish` removes the count before it moves the
epoch. We need: if the thread has a cell now, `Live() != 0`. The counter is a sum of adds and
subtracts that each thread makes in order (an `AddLive(n)` and its matching `RemoveLive(n)` are
ordered by coherence of the same thread's RMWs on one variable). In every prefix of the
modification order each other thread contributes a non-negative amount, because each of its
subtracts is preceded by the add it undoes, and this thread's own contribution is at least 1 while
it has a cell. A load that follows this thread's own `AddLive` returns a value from that prefix or
a later one, and later ones are also sums of such contributions. So it is at least 1. A load can
read a value larger than the thread's own count (other threads' cells), which only forces
`Recheck`.

**E4. Wrap-around (closed by 0342).** Until 0342 the counter was 32 bits and the adapter stored 32 bits
(`CellLink::m_seenEpoch`; the link is 16 bytes in every adapter). `NextEpoch` skips 0
(a new adapter's mark) and `UINT32_MAX` (the attached mark), so the period is 2^32 - 2 moves.
After exactly that many moves, an adapter that stored `v` and has not read since compares equal
again, and a cell promoted in the meantime is not found. The same holds for `ItemAdapters::quietEpoch`,
which then skips `Release` (a leaked frozen cell) and `Traverse` (a cycle the collector does not
see). It needs about 4.3 billion moves with no look by that adapter, and a promotion of its data in
that window. Each render that writes host data moves the epoch twice. The adapters that live that
long are the thread's globals snapshot and values the host keeps.

Since 0342 the counter, `ItemAdapters::quietEpoch` and `CellStep::epoch` are 64 bits, and the link
keeps two words: an attached link never compares its epoch (its reads take `Find`), and a detached
one has no position in a cell's list, so `CellLink::m_state` holds either the seen epoch or the
attached mark (bit 63) with the ownership bits and the position. The counter starts at 0, moves by
one, and would need 2^63 moves to reach the attached mark: 292 years at 10^9 moves a second, so it
never returns to a value an adapter stored, which is the unbounded counter the TLA+ model assumes.
The `epoch-wrap` mutant of the model (`MaxEpoch = 2`) keeps the old defect visible, and
`BorrowedCellsSingle.AdapterFindsACellPromotedAcrossThe32BitWrap` moves the counter past 2^32 at
once (`CellCounters::AdvanceEpochForTesting`) and fails with a 32-bit stored epoch.

**E5. The call id no longer guards anything.** Until #695, `m_exportedIn` kept the id of the call
that first shared a cell's items with the host and `Unshare` wrote in place when it equalled the
running call's; after 2^32 calls a stale mark could equal a new call (the first version of this
document filed that as task 0341). Call-end pinning replaced the mark with a flag, `m_shared`:
`BeginCall` still hands out `++m_lastCall`, but the id only names the call in the trace
(`borrowed_cells.h:541`) and a wrap changes nothing. The TLA+ model bounds it (`MaxCallId`).

## 4. Reference counts and the uniqueness tests (S2)

Reference counts are the only synchronization the library has between a kept value's reader and
the next writer of the same storage, so the proof of H3 is a proof about them.

**R1. `ValueObject` (the library's own count).** `intrusive_ptr_add_ref` is a relaxed add,
`intrusive_ptr_release` is an `acq_rel` subtract that destroys on 1, and `use_count()` is an
acquire load (`internal_value.h:132,162,166`). This is the standard pattern. A new reference is
only made from a reference the thread already holds to *the same object*, so the relaxed
increment cannot race with the destroying decrement. The `acq_rel` decrement makes everything the releasing thread did to the
object happen before the destruction on whichever thread drops the last reference. The acquire
`use_count()` gives the same edge to the tests built on it: `BorrowedCell::IsUnheld` (`use_count()
!= 1`, `owner->use_count() != 1`), `CycleCollector::DropUnshared` and the collector's reachability
test (`use_count() > internalRefs`). A reader's last access happens before its release, the
release is the last decrement the acquire load reads, so the writer or the collector sees it
happen before. The collector can read a count that is too high (a reference was dropped after the
read), which keeps an object alive and cannot free a live one: new references to an object are only
made from existing references to that same object (H6), so a count that was above `internalRefs`
stays above it unless the outside holder drops its reference, and then it is garbage anyway. "Only
from existing ones" alone is not enough for the walk: a thread that copies a reference to object B
from its reference to A and drops A between the walk's reads of A and B would make both look
internal. H6 is what excludes that; the counts' memory order does not.

**R2. `std::shared_ptr` counts are used as a uniqueness test and are not acquire.** Sites that ask
"is this the only reference" and then write or read the storage as if nobody else could:

| Site | Test | After it | Status on `42873f9` | After 0339 |
|---|---|---|---|---|
| `BorrowedCell::OwnsItemsAlone` (`borrowed_cells.cpp:187`), second conjunct: `OwnsMutableItemsAlone` (`internal_value.cpp:2325` list, `IsUnique` at `:1183`, used at `:2922`, dict) | `== 1` | a module call writes the cell's items in place (D8) | covered for a pin: the first conjunct is `m_object->use_count() == 1`, a `ValueObject` count with an acquire, and a call's pin holds that object | `BySharedMutable` storage is a `ValueObject`: both conjuncts are acquire loads (R1) |
| `Traverse` and `Clear` of the same two adapters (`internal_value.cpp:2339,2349,2925,2939`), run by the collector | `== 1` | `Clear` empties the container; `Traverse` reads its items only when it is the sole owner | open; TSan row `SharedOwnerTest.CollectorClearsAListTheHostDroppedOnAnotherThread` reports it | R1, same storage; the row is clean |
| `ChangeGlobals` (`template_env.cpp:485`) | `> 1` | `AddGlobal`/`RemoveGlobal` write the shared map in place when no render holds it | open; `SharedOwnerTest.AddGlobalAfterARenderOnAnotherThreadLetGoOfTheMap` reports it | `IsSoleOwner` (below); clean |
| `TraverseBoxed` (`internal_value.cpp:3904,3914`) | `== 1` on a callable's captures and attributes | the collector reads them | open | `IsSoleOwner`; reasoning only, no row (the collector only reads there) |
| `LoadsInPlace` (`template.cpp:86`) | `== 1` | the template parses into its implementation in place | open; `SharedOwnerTest.FirstLoadAfterACopyIsDroppedOnAnotherThread` reports it | `IsSoleOwner`; clean |

`std::shared_ptr::use_count()` is a relaxed load in libstdc++ (`bits/shared_ptr_base.h:230` says
"No memory barrier is used here so there is no synchronization with other threads") and in
libc++. The release side is fine: the other thread's drop of its last copy is an `acq_rel` (or a
release plus an acquire fence on the last owner). But a relaxed load that reads the value this
decrement wrote does not synchronize with it, so in the C++ memory model that thread's reads of the
storage are not ordered before the in-place write. That is a data race, which is undefined behaviour
on every platform; this document makes no claim about what a particular CPU or compiler turns it
into, because no weak-memory hardware was tested and the x86 memory order does not constrain the
compiler's transformations of unsynchronized accesses either. TSan reports it when it sees the
schedule (section 9). The fix is an acquire the checker can see: a library-owned count (R1), or one
acquire fence after the test reads 1 (`std::atomic_thread_fence(std::memory_order_acquire)`, which
TSan does not model). Task 0339.

**R3. The fix (0339).** The list and dict storage of a template's containers (`BySharedMutable`)
is now a `ValueObject`, so "alone" is R1's acquire load and costs no more than before. The other
three sites hold a `std::shared_ptr` the public API or the environment owns (a template's
implementation, the globals map, a callable's captures and attributes), so they test with
`IsSoleOwner` (`src/shared_owner.h`): after `use_count() == 1` it issues an acquire fence. The
last other owner's drop is a release decrement, and an acquire fence after a relaxed load that
read its value (or a later value of its release sequence, which only read-modify-writes extend)
synchronizes with it ([atomics.fences]/2), so the other owner's accesses happen before what the
caller does next. That costs nothing on x86. TSan does not model fences, so a TSan build copies the
pointer and drops the copy instead: an `acq_rel` decrement of the same count, which reads the last
value in the modification order and gives the same edge in a form TSan sees. (The first version
did the copy in every build; it cost 41 instructions per `Load`, +1.1% on `Load/plain_text`.)

The calls that carry their own acquire are not in the table: `BorrowedCell::IsUnheld` and
`CycleCollector::DropUnshared` test `ValueObject` counts. Task 0343's lint
(`scripts/use_count_check.py`) lists every `use_count()` call in `src/`, which is how the last
three rows were found. Everything else the argument needs from these counts holds: a reader holds a
reference for as long as it reads, so the count cannot be 1 while a reader is active, and nobody
can make a new reference to storage whose only holder is the module (a host cannot copy a value it
no longer holds).

## 5. Template module state (S3)

`TemplateModuleState` (`template_module.cpp`) holds the module's registry (`cells`), `callback`,
`module` (its context) and the `std::recursive_mutex mutex` that serializes calls
(`template_module.cpp:522`). The only locking site is `Call` (`template_module.cpp:450`),
`const std::scoped_lock lock(mutex)` held for the whole call, including `ModuleCallCollector`,
`CallScope` and the result copy (`ExportCopier().Copy`). So:

- **M1. Calls are mutually exclusive.** Everything in S3 is touched under the mutex, and the
  mutex gives the happens-before edge between one call's last write and the next call's first
  read, on whatever thread each call runs. A call nested through a host callback re-enters on the
  same thread (recursive mutex), where program order is the edge.
- **M2. Construction and destruction are outside the mutex, and need no lock.** `MakeModule`
  renders the top level and builds the exports before the state is shared (`make_shared`, then
  the exports capture the `shared_ptr`). The destructor runs when the last `shared_ptr` is
  released, by `shared_ptr`'s `acq_rel` decrement and on the thread that dropped it. No call can
  be running then, because a call is `ModuleCall::operator()`, which owns a `shared_ptr` to the
  state for its duration. The teardown collector (`template_module.cpp:327-333`) runs on that
  thread and is the only thing that touches the cells.
  The TLA+ model takes the teardown under the module lock (`cec3456`); that is a modelling
  convenience, because teardown cannot overlap a call and so needs no lock in the code.
- **M3. The result is copied out under the lock, and leaves it as a `Value`.** `ExportCopier`
  copies the call's result into a `Value` that does not refer to the module's context. What stays
  shared with the host is the exported storage of a cell (H3), which is the subject of section 6; section 10 lists where that does not hold.
- **M4. A kept value made under the lock is dropped on a thread that does not hold it.** The
  host may drop a kept value on any thread (H3b). A value that holds storage (`keep(l)`: in a
  module call a `CallExports` view, after the call the storage a pin recorded) destroys the value
  and its children through R1 counts and nothing of the registry. A value that holds an *adapter
  object* (`keep([l])`) reaches `CellLink::Unlink` from its destructor; since #695 that is a
  release of the link's reference to its cell (`borrowed_cells.cpp:295-316`), whichever thread runs
  it. The exception is a global's link, which is listed in the live cell and must be dropped on the
  render's thread (task 0346).
- **M5. The call ends under the lock.** `Call` holds the mutex across `ModuleCallCollector`, whose
  destructor pins the values the call handed out (`PinExportsOrSkip`) and drops the unheld cells
  before `EndCall`, and across `Complete()`, which pins and may throw so that the call fails
  rather than hand out values that read the host's data. So the pin is ordered before the next
  call by the mutex, and before the host's first read by the call's return (H3).

## 6. Publication of frozen cells and kept values (S4)

`BorrowedCells::Finish` runs at the end of the render (or on teardown of a module): it sets
`m_frozen` on every cell, detaches the global's links (the only ones listed in the cell), and
clears `m_links`; every other link already owns its cell by a reference count. A cell is read after
that by whichever threads hold a value that refers to it.

- **P1. What escapes a call is storage, or a view that is pinned when the call ends.**
  `ExportList`/`ExportMap` (`borrowed_cells.cpp:107-140`) return, inside a module call of the
  cell's own registry, a `CallExports` view that reads the cell on the call's thread until the call
  ends; `PinExports` then records the storage the values reach and sets the cells' `m_shared` flag,
  so that a later write copies unless the host let go (R2, `Unshare`). Outside a call they return a
  `GenericList`/`GenericMap` over the cell's storage accessor. A cell escapes as such only as a
  reference held by a `CellLink` that owns it, which a container holding an adapter object carries
  (`keep([l])`).
- **P2. After the freeze nothing writes the cell, its storage or its links.** `m_frozen` is a plain
  `bool`, written once by `Finish` on the render's thread. Every supported foreign access is ordered
  after it by H3: reading a kept value starts after the render completed, and the handoff is ordered
  after that. The one supported access that may run earlier, dropping or copying a reference (H3b),
  never reads `m_frozen` for an owning link: `Unlink` takes the link's own `OwnsBit`
  (`borrowed_cells.cpp:298`, "read by the link's own thread, never from the cell, which another
  thread may freeze meanwhile"). The plain reads of `m_frozen` that remain (`Inherit` at `:328`,
  `ExportingCall` at `:99`, `Unshare`) belong to reading a value, which H3 puts after the freeze.
- **P3. A frozen cell's reference count is the only thing foreign threads write.** `Inherit` adds a
  reference (relaxed, R1) and `Unlink` releases one (`acq_rel`, R1); neither writes the cell's
  other fields. A frozen cell can be destroyed by any thread, after the last holder's release
  (R1's edge), and its destructor reads only fields nothing writes.
- **P4. The collector walks a frozen cell only when the render's thread has noted it.** `Finish`
  hands the collector the frozen cells that a link still holds (`use_count() > 1`, acquire,
  `borrowed_cells.cpp:512`) and the collector reads them (`BorrowedCell::Traverse` reads
  `m_frozen`, `m_storage`, `m_retired`) on the same thread. A foreign thread may be reading the
  cell at the same time; both are reads. The walk writes only on objects it found unreachable
  from outside (`use_count() <= internalRefs`, an acquire read). A foreign reader holds a
  reference, so a cell it reads is counted as held from outside, and so is everything it reaches.
  The foreign thread cannot gain a reference to garbage, because it can only copy one it has.
  The `acquire` pairs with the readers' `acq_rel` release, so a reference dropped before the walk
  has its reads ordered before the `Clear`.

## 7. The environment and the globals snapshot (S5)

`TemplateEnvImpl` keeps `globalValues` (a `shared_ptr<ValuesMap>`, "Guarded by `guard`"), the
template caches and `globalsGeneration`.

- **G1. Writers hold the unique lock** (`ChangeGlobals`, `template_env.cpp:483`): the map is
  replaced by a copy when a render still holds it (R2; `IsSoleOwner` since 0339, R3), changed, and `globalsGeneration`
  is stored with `release`, still under the lock (`template_env.cpp:490`).
- **G2. A render takes its thread's snapshot of the globals** (`GetGlobalsSnapshot`,
  `template_impl.h:117`): it loads `globalsGeneration` with `acquire` and, if it differs from the
  snapshot's, takes the shared lock, reads the generation again (relaxed, which is consistent
  with the map because both change under the unique lock) and copies the `shared_ptr` to the map.
  The snapshot's adapters are views into that map and keep it alive (`LendGlobal`), so a later
  `AddGlobal` replaces the map and never writes the one a snapshot holds (H1).
- The `release`/`acquire` pair is not needed for the data (the lock orders it). It gives the
  host a guarantee it may rely on: a render that starts after `AddGlobal` returned and is ordered
  after it by the host's own synchronization sees the new generation. With relaxed ordering,
  coherence would give the same result once a happens-before edge exists. It does not need
  changing.
- `NewGlobalsGeneration`'s counter is a 64-bit relaxed `fetch_add` that only needs uniqueness.
  It does not wrap.

## 8. Invariants that keep the live cell single-threaded

The arguments of sections 3 to 6 need two structural invariants.

- **I1. A live cell's mutable state is touched only by the thread of its registry.** That state is
  `m_links`, `m_frozen` before the freeze, `m_shared`, the storage and the registry's table. A link
  belongs to an adapter *object*, which every `InternalValue` pointing at it shares by reference
  count, so a value the host keeps can hold the adapter that is attached to a live cell
  (`keep([l])`). The first version of this document found that dropping such a value on a host
  thread ran `Unlink` on the live cell (task 0340, TSan-reproduced on `81ce81a`). Since #695 the
  invariant holds by construction for every link but a global's: the link owns its cell
  (`OwnsBit`), so a drop is a release of a count, and the cell's `m_links` holds only globals'
  links. What is left is the global's link, which the thread's snapshot holds and `Finish`
  detaches: an item adapter of a global lives in a block the global's adapter lets go of when it
  reads another block, and a host value may then be the block's only holder, so a host thread's
  drop edits `m_links` of the live cell (task 0346, found by reading, not reproduced).
- **I2. The registry is thread-local.** `CellsForWrite` and `CurrentCells` read `s_current`
  (`cycle_collector.h:103`), so `Find` and `Adopt` cannot reach another thread's table. A
  module's registry is `m_external`, reached only from the call that holds the mutex (M1). This
  holds in the code; the model's `Foreign` flag checks the same property for reads (a read through
  an adapter whose cell belongs to another registry).

## 9. Evidence

Current results ran on master `42873f9` (after #694, #695 and #699) with clang 18.1.3, x86-64 Linux,
`RelWithDebInfo`, `-DJINJA2CPP_WITH_SANITIZERS=thread`, `JINJA2CPP_NODEREF_CHECKS=FULL` (the sanitizer
default), in `build-tsan/`. TSan finds the races of the schedules it sees and models the C++ memory
model, but x86 never reorders a load past a later store, no weak-memory machine (ARM, POWER) was
run, and TSan ignores `atomic_thread_fence` (clang warns with `-Wtsan`). So a clean run is evidence,
not proof.

**Existing rows.** `build-tsan/jinja2cpp_tests --gtest_filter='BorrowedCells*:EmbeddingLifecycle*:*Module*:*Thread*:*Concurrent*'`:
133 tests, all pass, **no TSan report**. They cover: a kept view rendered from several threads, a
kept global read while its thread writes (T2), a frozen value read from four threads (T4), module
calls from two threads while a third reads the first call's value (D8), concurrent renders sharing
host data, and the embedding-lifecycle rows C1 to C6. They leave out what the probes below test: the
host thread *drops* the last reference to a kept value, which is when the reference counts have to
carry the ordering.

Two probes, kept in `/mnt/project-files/0338-memory-order/` (`probe_use_count.cpp` is the first
version, `probe_use_count2.cpp` the corrected one; `probe_unlink.cpp`; about 80 lines each, built
against `build-tsan/libjinja2cpp.a`). They are not in the repository; the tasks that fix a gap add
their own rows.

**Probe A, the uniqueness test.** A module macro appends to `l` and calls `keep(l)`. A second thread
takes each kept value (through a mutex), reads its items and drops it. The main thread calls the macro
2000 times.

| Master | Variant | Runs | TSan reports per run |
|---|---|---|---|
| `81ce81a` | `l = [0]`, reader starts while the call runs | 3 | 1, 6, 5 (write in `ListAppend` against the reader's `GetItem`) |
| `81ce81a` | the same with `OwnsMutableItemsAlone` and `IsUnique` reading the count with `__ATOMIC_ACQUIRE` (experiment, reverted) | 3 | 0, 0, 0 |
| `81ce81a` | the same with `std::atomic_thread_fence(acquire)` after `use_count() == 1` | 3 | 5, 5, 5 (TSan does not model the fence; says nothing) |
| `42873f9` | the first probe unchanged | 3 | 25 to 50: the reader read the kept value while the call that produced it was still running, which the host contract (H3) does not allow |
| `42873f9` | `probe_use_count2.cpp`: the value reaches the reader only after the call returned, `l = [0]` and `l = [[0]]` | 3 each | **0, 0, 0** |

The first probe was out of contract: the early `81ce81a` results mixed a real race (the relaxed
`shared_ptr` count) with that misuse, and only the acquire-load row separated them. After #695 the
kept value holds the cell's `ValueObject`, and `OwnsItemsAlone` tests its acquire count first
(R1), so the module path no longer depends on the `shared_ptr` count. The `shared_ptr` count remains
the only test at the sites listed in gap 1 until 0339 (rows below).

**Probe B, the foreign unlink.** The template does `l.append(1)`, then `keep([l, [l]])`, then 30 more
appends. A second thread drops each kept container as soon as it can. On `81ce81a`, 20000 renders
gave one TSan report in each of 3 runs: a read of `BorrowedCell::m_frozen` in `CellLink::Unlink` on
the dropper thread against the write in `BorrowedCells::Finish`. On `42873f9`, same probe, **0
reports in 3 of 3 runs**: the owning links of #695 make the kept container hold the cell, so the
drop no longer reaches `Unlink` on a live cell (I1).

**Task 0339 rows.** Three tests in `test/borrowed_cells_test.cpp` (`SharedOwnerTest.*`) let
another thread read the shared storage and drop it with no happens-before edge back (a relaxed
flag), and then take the in-place path: the collector's `Clear`, the first `Load` and `AddGlobal`.
On the parent of 0339 each reports a TSan data race in 3 of 3 runs; with the fix, 0 of 3. Getting
them to fail took care, because any other count the dropping thread touches carries the edge:
reading an item of the list holds and lets go of the item's `ValueObject`, whose count the
collector reads with an acquire, and a copy of the template dropped on the testing thread is
itself an `acq_rel` decrement that orders the other thread's drop (the mechanism of R3).

What the evidence does not cover: weak-memory hardware; `TraverseBoxed` (reasoning only); the
counter wrap (gap 3: reasoning only, it needs 2^32 moves); and the cycle collector's walk,
which the existing rows exercise without a report.

## 10. Gaps and tasks

| # | Gap | Task | How found | Severity |
|---|---|---|---|---|
| 1 | The `shared_ptr::use_count()` uniqueness tests at `IsUnique`, the list/dict `Traverse` and `Clear`, `TraverseBoxed`, `LoadsInPlace` and `ChangeGlobals` are relaxed: the other holder's last access is not ordered before the in-place access | [0339](../tasks/0339-shared-ptr-uniqueness-needs-acquire.md), **closed** (R3) | probe A on `81ce81a`, reading; TSan rows in 0339 | low: a formal data race on every platform; the module path is closed by #695, the rest needs a host thread dropping at the same time as the collector, a load or a globals change |
| 2 | The global's own link (the only link in `cell->m_links`) is still unlinked outside the owning thread when the host drops it | [0346](../tasks/0346-global-item-link-dropped-on-another-thread.md) | reading after #695 | low; see the task |
| 3 | The 32-bit epoch wraps after 2^32 - 2 moves: an adapter or `ItemAdapters` block that stored `v` and was not read since can miss a cell, skip a `Release` or skip a `Traverse` | [0342](../tasks/0342-epoch-wraparound.md), **closed** (E4) | reading; TLA+ `epoch-wrap` | low: needs 2^31 renders that write host data between two reads of one long-lived adapter |

Closed by #695 (`done` in their task files): [0340](../tasks/0340-foreign-unlink-of-a-live-cell.md),
the foreign unlink of a live cell, found by probe B (owning links); and
[0341](../tasks/0341-call-id-wraparound.md), the call-id wrap (the call id is now only a trace name
and the `m_shared` flag decides in-place writes). Tasks 0332 and 0335 are closed as well, so the
nested rows of the first probe are no longer a separate gap.

Hand-off to the other steps of 0338: step A's model covers the owning-link transitions;
step B's trace validation should log the epoch as a full-width integer so that a wrap is visible.

## 11. Keeping it true

- A new atomic, lock or `thread_local` in `src/borrowed_cells*`, `src/cycle_collector*`,
  `src/template_module.cpp` or `src/internal_value.*` updates section 12 in the same PR. Review
  rejects a new `memory_order` weaker than `acq_rel` on a count that guards destruction, and a new
  uniqueness test on a `std::shared_ptr` count (use R1's `ValueObject` count; `scripts/use_count_check.py` (0343) enforces it, and a correct call carries a `// use_count: <reason>` comment).
- A change to who may hold a live `CellLink` (I1) or to how the registry is found (I2) changes
  section 8 first.
- The TLA+ model keeps its "Not modelled" bullet on memory order and points here.
- If a counter ever carries data (a value written before it and read after it), this argument no
  longer covers it; check it with a C11 model checker (GenMC or CDSChecker) on an extracted kernel,
  as 0338 proposes.

## 12. Every atomic and lock

Every atomic, lock and thread-local the protocol, the collector or the module code touches, with
what it orders and whether section 3 to 8 covers it. The first block is the protocol's own; the
second is state in the same translation units that the protocol does not depend on, listed so that a
reader does not have to search for it.

| Where | What | Order | Role | Verdict |
|---|---|---|---|---|
| `borrowed_cells.h:52,55,62,63,65,66`, `.cpp:39,48` | `CellCounters::s_epoch`: load, `fetch_add` (64 bits) | relaxed | change hint for adapters, blocks, enumerations | E1, E2 hold; no wrap since 0342 (E4) |
| `borrowed_cells.h:51,58,59,62`, `.cpp:28` | `CellCounters::s_live`: load, `fetch_add`, `fetch_sub` | relaxed | quiet-block hint | E3 holds; a 32-bit sum, no wrap in practice |
| `internal_value.h:162,183` | `ValueObject::m_refCount` add | relaxed | new reference from an existing one | R1 holds |
| `internal_value.h:166` | `m_refCount` release | `acq_rel`, destroys on 1 | end of a reader's access, destruction | R1 holds |
| `internal_value.h:132` | `use_count()` | acquire | collector reachability, `DropUnshared`, `IsUnheld` | R1 holds |
| `internal_value.cpp` `BySharedMutable::IsUnique` | storage `ValueObject::use_count() == 1` | acquire | in-place write, collector `Traverse`/`Clear` of template lists and dicts | R1 holds (0339) |
| `shared_owner.h` `IsSoleOwner`, at `TraverseBoxed`, `LoadsInPlace`, `ChangeGlobals` | `shared_ptr::use_count() == 1`, then an acquire fence (TSan builds: a copy dropped) | relaxed load + acquire fence | collector walk of a callable, in-place Load, in-place globals change | R3 holds (0339) |
| `borrowed_cells.cpp:99,187,240,295-316,478-528` | plain `m_frozen`, `m_links`, `m_shared` | none; single thread by I1 (owning links, #695) | live-cell state | I1 holds except the global's own link: **gap 2** |
| `borrowed_cells.cpp:187` | `ValueObject::use_count()` first in `OwnsItemsAlone` | acquire | module in-place write | R1 holds |
| `cell_trace.cpp:44,174,179` | `m_mutex`, `nextThread` | mutex; relaxed id | trace log (test builds) | no protocol data |
| `template_module.cpp:450,523` | `recursive_mutex` | lock | module calls, S3 | M1 holds |
| `thread_annotations.h:51-75`, `detail::SharedMutex`, `ReaderLock`, `WriterLock` | annotated wrappers of the shared mutexes in the environment | shared/unique | clang `-Wthread-safety` checks the guarded members (0343) | G1, G2 |
| `template_env.cpp:483`, `template_env_impl.h:86` | `globalsGeneration` store | release, under the unique lock | new globals | G1 |
| `template_impl.h:122` | `globalsGeneration` load | acquire, no lock | is the cached snapshot current | G2 (acquire not needed for the data) |
| `template_impl.h:127` | `globalsGeneration` load | relaxed, under the shared lock | the snapshot's generation | G2 |
| `template_env.cpp:473` | `NewGlobalsGeneration` `fetch_add` | relaxed | unique 64-bit id | needs uniqueness only; no wrap |
| `cycle_collector.h:103` | `thread_local s_current` | none | the render's collector and registry | I2 |
| `internal_value.cpp:78` | `thread_local t_pendingDestroy` | none | deferred destruction list | thread-owned; an object made elsewhere is deleted here (R1 gives the edge) |
| `internal_value.cpp:283` | `thread_local g_bridgingAccessor` | none | bridging a map accessor during a call | thread-owned |

Outside the protocol (counters and caches; none carries data across threads):

| Where | What | Order |
|---|---|---|
| `template_impl.h:506,539,1050` | `m_outputSizeHint.value` | relaxed; a size hint |
| `render_context.h:205-207` | `NewSlot` per-bucket round-robin | relaxed; a slot choice |
| `render_context.h:346-347` | `lastSerial` scope stamp | relaxed; 32 bit, wrap documented at the declaration |
| `render_context.h:1503-1504` | `NewEvalContextId` | relaxed; 32 bit |
| `statements.cpp:1418` | `ForStatement::NewLoopId` | seq_cst `++`; a 64-bit id |
| `value.h:1082,1086,1134`, `internal_value.cpp:262` | `UserCallable::m_gen` | seq_cst `++`; an identity counter |
| `error_info.h:142,155` | error payload refs | relaxed add, `acq_rel` sub; the R1 pattern |
| `template_env.cpp:236-269`, `template_env_impl.h:104` | `m_loadSettingsGuard` | mutex |
| `template_env.cpp:336,392,483,506-542` and `template_module.cpp:369`, `template_impl.h:127` | `guard` | shared_timed_mutex, unique for writers, shared for readers |
| `template_impl.h:592,1055` | `m_metadataMutex` | mutex |
| `render_workspace.h:44,217`, `render_context.h:184`, `recursion_guard.h:63,78,123,406`, `render_failure.cpp:154`, `markup.h:66`, `template_slots.h:155` | `thread_local` caches, limits, failure, buffers | none; thread-owned |
