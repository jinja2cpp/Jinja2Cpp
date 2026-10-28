# Memory order of the borrowed-cell protocol

Written 2026-10-28 on master `81ce81a`, step C of the route in
[0338](../tasks/0338-borrowed-cell-protocol-safety-proof.md) (item 5, "memory order by design and
argument, not in TLA+"). It covers `src/borrowed_cells.{h,cpp}`, `src/cycle_collector.{h,cpp}` and
`src/template_module.cpp`, and the state they share with the rest of the library. It changes no
code. What it finds is filed as tasks 0339 to 0342, listed in section 7.

The claim is narrow. **Given the host contract in section 1, every access the protocol makes to
state that more than one thread can reach is either ordered by a lock, ordered by a
happens-before edge the library creates (an acquire/release pair on a reference count), or reads
state nothing writes any more.** Two places do not meet that claim today, and TSan reproduces
both (section 9): the uniqueness tests on `std::shared_ptr` counts have no acquire (task 0339), and a
host thread that drops a kept container can unlink an adapter from a live cell (task 0340). Two
more are bounded only by the width of a counter (tasks 0341 and 0342). The TLA+ model in `tla/` says
nothing about any of this: it interleaves atomic steps, so it assumes sequential consistency
(`tla/README.md`, "Not modelled").

An analogy for the shape of the argument. The library is a print shop with one proof room per
render. Each room's table (the render's registry of cells) is only ever touched by the person in
that room. Three things cross the walls: a notice board with two numbers on it (the counters, and
anyone may glance at them), finished proofs carried out the door (frozen cells and kept values),
and the shop's reference binders (the module's state and the environment, behind a sign-out
sheet, which is the mutex). Nobody ever edits a proof once it is carried out; when the shop wants
to reuse a proof's paper, someone has to check that the last reader has put it down. That check is
the one that is not rigorous yet (gap 1).

## 1. Host contract the argument rests on

These are the host behaviours the protocol supports. A host that breaks one gets a data race the
library does not promise to prevent.

- **H1. Host data is immutable while any render can read it.** Lists and maps passed as params,
  environment globals and values a host callable returns are not written by the host during a
  render, and nothing the library does writes them (decision D2 of 0049). The environment's
  globals are changed through `TemplateEnv`, which copies on change (section 7).
- **H2. A render and its adapters live on one thread.** The adapters of the host's data, the
  render's registry of cells and its collector are used by the thread that runs the render. A
  value handed to a callback is valid during the call and is used on the callback's thread. A
  value the host keeps after the call is a snapshot (H3).
- **H3. A kept value is a snapshot.** A value that a render or a module call returns, or hands to
  the host with a callback argument, is immutable once the call that handed it out has returned
  (decisions D4 and D8 of 0331). The host may pass it to other threads with its own
  synchronization (a mutex, a queue, a thread start), and may drop it on any thread. Two cases
  do not meet this today: a container the call made that holds a template variable's adapter
  (`keep([l])`, task 0332) is not a snapshot while the module's cell lives, and dropping such a
  container on another thread races with the end of the render (task 0340).
- **H4. Concurrent renders share only H1 data and kept values.** Two threads rendering with the
  same `Template` or `TemplateEnv` share the parse tree, the environment's callables and
  settings, and the globals map, all read-only during a render (loads and `AddGlobal` take the
  environment's lock).
- **H5. Template module calls take turns.** One module's calls are serialized by its mutex; a
  call nested through a host callback runs on the thread that holds it.

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
  argument; section 8 states what keeps it true, and the one window where it does not (gap 2).
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
src/borrowed_cells.h:48   Epoch()        s_epoch.load(relaxed)
src/borrowed_cells.h:51   Live()         s_live.load(relaxed)
src/borrowed_cells.h:58   AddLive(n)     s_live.fetch_add(relaxed)
src/borrowed_cells.h:59   RemoveLive(n)  s_live.fetch_sub(relaxed)
src/borrowed_cells.cpp:34 NextEpoch()    s_epoch.load(relaxed) + compare_exchange_weak(relaxed)
```

The counters carry no data. Nothing is published through them: a reader that sees a value does not
go on to read anything that a writer wrote before writing the counter. They are hints that decide
whether to take a slower path, and the slower path takes its own look at the registry. That is
what makes relaxed enough, and the argument has to show that no wrong hint can lead to a wrong
answer.

**What an adapter does with the epoch.** `CellLink::Get` returns null without a lookup when
`m_seenEpoch == Epoch()`. `Find` stores the epoch it read *before* it looks the cell up
(`borrowed_cells.cpp:189`), so a promotion after that read moves the epoch past the stored value
and the next read looks again. `ItemAdapters` and `CellStep` store an epoch for a whole block or
enumeration with the same rule.

**E1. A thread never misses its own promotion.** The registry that the thread's adapters consult
is the thread's own, so only that thread's promotions (`Adopt`) and render ends (`Finish`) matter
to them. Take an adapter that stored epoch `v` at time t1, and a promotion by the same thread at
t2 > t1 that moved the counter to `v'`. Both are in `s_epoch`'s modification order, and `v'` is
after `v` in it, because `v'` was made by a read-modify-write that observed `v` or a later value
(`NextEpoch` is a CAS loop; each iteration observes the latest value in the modification order
for its own CAS to succeed). Coherence (read-read, write-read) says a load that follows the
promotion in program order returns `v'` or a later value. Later values are larger, apart from the
wrap-around (E4), so the load is not `v`, and the adapter looks again. A block destroyed on
another thread (the host drops a kept value) compares with a load that happens after the handoff,
so it sees every promotion made before the handoff; one made after is gap 2's window.

**E2. Other threads' moves cost a lookup and nothing else.** A promotion on another thread moves
the epoch too, so a quiet adapter takes the `Find` path once and finds nothing in its own
registry. That is a performance cost, not a correctness one: the lookup is a read of the thread's
own table. A stale (old) epoch value read because of another thread's promotion can only be a
value the adapter already compares unequal or equal to for E1's reason.

**E3. `Live` is read after the epoch, for the same thread's cells.** `QuietEpoch` (`borrowed_cells.h:293`)
returns the epoch only if `Live() == 0`, so that a block of item adapters made while no cell
exists can skip all its slots until the epoch moves. `Adopt` counts the cell (`AddLive`) before it
moves the epoch (`borrowed_cells.cpp:304-306`), and `Finish` removes the count before it moves the
epoch. We need: if the thread has a cell now, `Live() != 0`. The counter is a sum of adds and
subtracts that each thread makes in order (an `AddLive(n)` and its matching `RemoveLive(n)` are
ordered by coherence of the same thread's RMWs on one variable). In every prefix of the
modification order each other thread contributes a non-negative amount, because each of its
subtracts is preceded by the add it undoes, and this thread's own contribution is at least 1 while
it has a cell. A load that follows this thread's own `AddLive` returns a value from that prefix or
a later one, and later ones are also sums of such contributions. So it is at least 1. A load can
read a value larger than the thread's own count (other threads' cells), which only forces
`Recheck`.

**E4. Wrap-around.** The counter is 32 bits and the adapter stores 32 bits
(`CellLink::m_seenEpoch`; the link is 16 bytes in every adapter). `NextEpoch` skips 0
(a new adapter's mark) and `UINT32_MAX` (the attached mark), so the period is 2^32 - 2 moves.
After exactly that many moves, an adapter that stored `v` and has not read since compares equal
again, and a cell promoted in the meantime is not found. The same holds for `ItemAdapters::quietEpoch`,
which then skips `Release` (a leaked frozen cell) and `Traverse` (a cycle the collector does not
see). It needs about 4.3 billion moves with no look by that adapter, and a promotion of its data in
that window. Each render that writes host data moves the epoch twice. The adapters that live that
long are the thread's globals snapshot and values the host keeps. This is the lowest-severity gap
(task 0342) but it is not covered by the argument, and the TLA+ model's unbounded counter hides
it.

**E5. The call id has the same defect with a worse consequence.** `BeginCall` hands out `++m_lastCall`
and skips only 0 (`borrowed_cells.h:373`). `m_exportedIn` keeps the id of the call that first
shared a cell's items with the host; `Unshare` treats `m_exportedIn == m_call` as "the call that
handed them out is writing". After 2^32 calls of one module with no write to that cell, a stale mark
equals the new call's id, the write goes in place, and a kept value that still shares the items
changes, which is a data race with the host thread that reads it (task 0341).

## 4. Reference counts and the uniqueness tests (S2)

Reference counts are the only synchronization the library has between a kept value's reader and
the next writer of the same storage, so the proof of H3 is a proof about them.

**R1. `ValueObject` (the library's own count).** `intrusive_ptr_add_ref` is a relaxed add,
`intrusive_ptr_release` is an `acq_rel` subtract that destroys on 1, and `use_count()` is an
acquire load (`internal_value.h:132,162,166`). This is the standard pattern. A new reference is
only made from a reference the thread already holds, so the relaxed increment cannot race with the
destroying decrement. The `acq_rel` decrement makes everything the releasing thread did to the
object happen before the destruction on whichever thread drops the last reference. The acquire
`use_count()` gives the same edge to the tests built on it: `BorrowedCell::IsUnheld` (`use_count()
!= 1`, `owner->use_count() != 1`), `CycleCollector::DropUnshared` and the collector's reachability
test (`use_count() > internalRefs`). A reader's last access happens before its release, the
release is the last decrement the acquire load reads, so the writer or the collector sees it
happen before. The collector can read a count that is too high (a reference was dropped after the
read), which keeps an object alive and cannot free a live one: new references are only made from
existing ones, so a count that was above `internalRefs` stays above it unless the outside holder
drops its reference, and then it is garbage anyway.

**R2. `std::shared_ptr` counts are used as a uniqueness test and are not acquire.** Three sites ask
"is this the only reference" and then write the storage in place:

| Site | Test | In place after it |
|---|---|---|
| `BorrowedCell::Unshare` via `OwnsMutableItemsAlone` (`internal_value.cpp:2000`, list: `std::shared_ptr<InternalValueList>::use_count()`; `internal_value.cpp:1183,2590`, dict: `BySharedMutable::IsUnique`) | `== 1` | the module call writes the cell's items (D8) |
| `Traverse` and `Clear` of the same two adapters (`internal_value.cpp:2007,2017,2593,2607`), run by the collector | `== 1` | `Clear` empties the container; `Traverse` reads its items only when it is the sole owner |
| `ChangeGlobals` (`template_env.cpp:485`) | `> 1` | `AddGlobal`/`RemoveGlobal` write the shared map in place when no render holds it |

`std::shared_ptr::use_count()` is a relaxed load in libstdc++ (`bits/shared_ptr_base.h:230` says
"No memory barrier is used here so there is no synchronization with other threads") and in
libc++. The release side is correct: the host thread's drop of its last copy is an `acq_rel`
(or release plus an acquire fence on the last owner). But a relaxed load that reads the value this
decrement wrote does not synchronize with it, so in the C++ memory model the host thread's reads of
the items are not ordered before the module's in-place write. That is a data race by definition
(TSan reports it when it sees one: section 9), though it needs weak ordering to show up as a wrong
value, since on x86 loads are not reordered with later stores and on ARM a control dependency from
the load to the write orders them in practice. The fix is one acquire fence after the test reads 1
(`std::atomic_thread_fence(std::memory_order_acquire)`, which together with the release
decrement gives the synchronizes-with edge), or switching the holder to a count the library
owns (R1). Task 0339.

Everything else the argument needs from these counts holds: a kept value's reader holds a
reference for as long as it reads, so the count cannot be 1 while a reader is active, and nobody
can make a new reference to storage whose only holder is the module (H3: a host cannot copy a
value it no longer holds).

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
  host drops a kept value on any thread. For a value that holds only storage (`keep(l)` after the
  call: `ExportList` returns an adapter over the cell's *storage*, which owns its items) the
  destruction touches the value and its children through R1 counts and nothing of the registry.
  A value that holds an *adapter object* (`keep([l])`) reaches `CellLink::Unlink` from its
  destructor. That is safe when the cell is frozen (the unlink only releases a reference, P3). It
  is not safe when the cell is live, and section 8 (I1) says when that can happen.

## 6. Publication of frozen cells and kept values (S4)

`BorrowedCells::Finish` runs at the end of the render (or on teardown of a module): it sets
`m_frozen` on every cell, detaches the global's links, keeps the others as owning references, and
clears `m_links`. A cell is read after that by whichever threads hold a value that refers to it.

- **P1. What escapes a call is storage, not the registry's cell.** A value escapes through a
  call's return, an `ExportCopier` result, `keep(...)` in a callback or a module export.
  `ExportList`/`ExportMap` (`borrowed_cells.cpp:85-95`) return a `GenericList`/`GenericMap` over the
  cell's *storage* accessor, which is plain template-owned storage; `NoteExport` records that it is
  shared (`m_exportedIn`) and the next write copies it when a reader still holds it (`Unshare`). A
  frozen cell escapes only as a reference held by a `CellLink` copy (`Inherit`), made after the
  freeze. The exception is a container that holds an adapter object itself (`keep([l])`, 0332): it
  escapes with the adapter's link, attached to a live cell.
- **P2. After the freeze nothing writes the cell, its storage or its links.** `m_frozen` is a plain
  `bool`, written once by `Finish` on the render's thread. A value that escaped *after* the freeze (a
  render's return value, a module export from a finished call) is ordered after it by the host's
  handoff (H3). A value that escaped *before* it and holds only storage never reads `m_frozen`, so
  the missing order is harmless. A value that escaped before it holding an attached adapter reads
  `m_frozen` when it is destroyed or copied (`Unlink`, `Inherit`), concurrently with the freeze;
  that is gap 2 (section 8, task 0340).
- **P3. A frozen cell's reference count is the only thing foreign threads write.** `Inherit`
  (`borrowed_cells.cpp:246`) adds a reference (relaxed, R1), `Unlink` releases one (`acq_rel`,
  R1); neither touches the cell's other fields, because `Unlink` returns after the release when
  `m_frozen` is set. A frozen cell can be destroyed by any thread, after the last reader's release
  (R1's edge), and its destructor reads only fields nothing writes.
- **P4. The collector walks a frozen cell only when the render's thread has noted it.** `Finish`
  hands the collector the frozen cells that a link still holds (`use_count() > 1`, acquire,
  `borrowed_cells.cpp:404`) and the collector reads them (`BorrowedCell::Traverse` reads
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
  replaced by a copy when a render still holds it (R2, gap 1), changed, and `globalsGeneration`
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

The arguments of sections 3 to 6 need two structural invariants. I2 holds. I1 holds for modules
and for the render's own frames, and does not hold in one window, which is gap 2.

- **I1. The last reference to an adapter object attached to a live cell is dropped on the cell's
  thread.** A link belongs to an adapter *object*, a `ValueObject` that every `InternalValue`
  pointing at it shares by reference count, not to the values. `CellLink`'s copy constructor
  detaches (`Inherit` does nothing for a live cell), but copying an `InternalValue` does not copy
  the adapter, so a container the host keeps can hold the very adapter that is attached
  (`keep([l])`, 0332). What makes the last reference the cell thread's:
  - for a **module**, the module's context holds `l` until the destructor, which freezes the cells
    first (`template_module.cpp:327-334`, `cells.Release` before `module.reset()`), so a host
    reference is never the last while the cell is live;
  - for a **render**, the frames hold it until they are destroyed, which is *before* the
    collector's `Finish` freezes the registry (`CycleCollector` is made first so that it ends
    after everything the render keeps values in). Between those two points a kept container is
    the only holder, and a host thread that drops it runs `~OwnedAdapter` → `Unlink` on a live
    cell. `Unlink` reads the plain `bool m_frozen` while `Finish` writes it, and, when it reads
    `false`, edits `m_links` while `Finish` walks it. TSan reports the first as a data race (probe
    B, section 9); the second would corrupt the vector. This is gap 2 (task 0340).
- **I2. The registry is thread-local.** `CellsForWrite` and `CurrentCells` read `s_current`
  (`cycle_collector.h:103`), so `Find` and `Adopt` cannot reach another thread's table. A
  module's registry is `m_external`, reached only from the call that holds the mutex (M1). This
  holds in the code; the model's `Foreign` flag checks the same property for reads (a read through
  an adapter whose cell belongs to another registry).

## 9. Evidence

Everything here ran on master `81ce81a` with clang 18.1.3, x86-64 Linux, `RelWithDebInfo`,
`-DJINJA2CPP_WITH_SANITIZERS=thread`, `JINJA2CPP_NODEREF_CHECKS=FULL` (the sanitizer default), in
`build-tsan/`. TSan finds the races of the schedules it sees and models the C++ memory model, but x86
never reorders a load past a later store and TSan ignores `atomic_thread_fence` (clang warns with
`-Wtsan`). So a clean run is evidence, not proof, and the fence experiment below cannot be judged
by TSan.

**Existing rows.** `build-tsan/jinja2cpp_tests --gtest_filter='BorrowedCells*:EmbeddingLifecycle*:*Module*:*Thread*:*Concurrent*'`:
121 tests in 16 suites, all pass, **no TSan report**. They cover: a kept view rendered from several
threads, a kept global read while its thread writes (T2), a frozen value read from four threads
(T4), module calls from two threads while a third reads the first call's value (D8), concurrent
renders sharing host data, and the embedding-lifecycle rows C1 to C6 (concurrent loads, renders,
module macros, environment teardown). What they leave out is exactly what the probes below test: the
host thread *drops* the last reference to a kept value, which is when the reference counts have to
carry the ordering.

Two probes follow, kept in the project files
(`/mnt/project-files/0338-memory-order/`: `probe_use_count.cpp`, `probe_unlink.cpp`; ~80 lines each,
built against `build-tsan/libjinja2cpp.a`). They are not in the repository; the tasks that fix the
gaps add their own rows.

**Probe A, the uniqueness test (gap 1).** A module macro appends to `l` and calls `keep(l)`. A
second thread takes each kept value (through a mutex, so the handoff is ordered), reads its items,
and drops it outside the lock. The main thread calls the macro 2000 times. The module's next write
sees one reference and writes in place:

| Variant | Runs | TSan reports per run |
|---|---|---|
| master, `l = [0]` | 3 | 1, 6, 5 (write in `ListAppend`'s `push_back` against the reader's `GetItem`) |
| `OwnsMutableItemsAlone` and `IsUnique` read the count with `__ATOMIC_ACQUIRE` (experiment, reverted; reads libstdc++'s `_M_use_count` directly) | 3 | 0, 0, 0 |
| the same with `std::atomic_thread_fence(acquire)` after `use_count() == 1` | 3 | 5, 5, 5 (TSan does not model the fence; this row says nothing) |
| master, `l = [[0]]`, macro writes `l[0]` | 3 | 10 each; **still 7 to 10 with the acquire load**: the reader reads the inner list's live cell through the kept outer list, which is 0332, not gap 1 |

The acquire-load row isolates the cause: with a load TSan understands, every report of the flat
variant disappears. The reports are the formal race of section 4 (R2), seen on x86 because TSan
checks happens-before, not the hardware's behaviour.

**Probe B, the foreign unlink (gap 2).** The template does `l.append(1)`, then `keep([l, [l]])`, then
30 more appends. The callback queues the kept container; a second thread drops it as soon as it
can. 20000 renders: **one TSan report in each of 3 runs**: a read of `BorrowedCell::m_frozen` in
`CellLink::Unlink` (from `~OwnedAdapter`, on the dropper thread) against the write in
`BorrowedCells::Finish`. The read that returned `false` would go on to edit `m_links` while `Finish`
walks it, which the report does not show but the code does (`borrowed_cells.cpp:230-235`).

What the evidence does not cover: the weak-memory hardware behaviour of any of this (no ARM or
POWER run); `ChangeGlobals` (gap 1, third site: reasoning only, because a render thread keeps its
cached snapshot, so the in-place branch needs a thread that has just exited or called
`ReleaseThreadCaches`); the counter wraps (gaps 3 and 4: reasoning only, they need 2^32 moves);
and the cycle collector's walk, which the existing rows exercise without a report.

## 10. Gaps and tasks

| # | Gap | Task | How found | Severity |
|---|---|---|---|---|
| 1 | The uniqueness tests (`OwnsMutableItemsAlone`, `BySharedMutable::IsUnique`, `ChangeGlobals`) read a relaxed `shared_ptr::use_count()`; the host's last read of a kept value does not happen before the module's next in-place write | [0339](../tasks/0339-shared-ptr-uniqueness-needs-acquire.md) | probe A, TSan | medium: a formal data race on every platform, a wrong value only on weak-memory hardware |
| 2 | A host thread that drops a kept container holding an attached adapter calls `CellLink::Unlink` on a live cell, racing `Finish` (and editing `m_links` if it reads `m_frozen == false`) | [0340](../tasks/0340-foreign-unlink-of-a-live-cell.md) | probe B, TSan | medium to high: a possible vector corruption, needs the host to drop in the window after the frames die |
| 3 | `m_lastCall` wraps after 2^32 calls and a stale `m_exportedIn` equals a new call's id: a write goes in place into items a kept value shares | [0341](../tasks/0341-call-id-wraparound.md) | reading | low: needs 2^32 calls of one module with no write to that cell in between; the fix is a few lines (depends on 0337's sentinel) |
| 4 | The 32-bit epoch wraps after 2^32 - 2 moves: an adapter or `ItemAdapters` block that stored `v` and was not read since can miss a cell, skip a `Release` or skip a `Traverse` | [0342](../tasks/0342-epoch-wraparound.md) | reading | low: needs 2^31 renders that write host data between two reads of one long-lived adapter |

Not new: the nested rows of probe A are task 0332 (a kept container holds the module's live cell), which this
document confirms is a data race under TSan and not only a stale value. Its fix (freeze at the end
of every call) also closes gap 2 for modules; the render's window remains.

Hand-off to the other steps of 0338: step A should model the foreign drop (an `Unlink` by a thread
that does not own the registry) and the call-id wrap as transitions, and step B's trace
validation should log the epoch and call ids as full-width integers so that a wrap is visible.

## 11. Keeping it true

- A new atomic, lock or `thread_local` in `src/borrowed_cells*`, `src/cycle_collector*`,
  `src/template_module.cpp` or `src/internal_value.*` updates section 12 in the same PR. Review
  rejects a new `memory_order` weaker than `acq_rel` on a count that guards destruction, and a new
  uniqueness test on a `std::shared_ptr` count (use R1's `ValueObject` count or add the fence).
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
| `borrowed_cells.h:48,61`, `.cpp:27,34,43` | `CellCounters::s_epoch`: load, CAS loop | relaxed | change hint for adapters, blocks, enumerations | E1, E2 hold; wrap = gap 4 |
| `borrowed_cells.h:51,58,59,62`, `.cpp:28` | `CellCounters::s_live`: load, `fetch_add`, `fetch_sub` | relaxed | quiet-block hint | E3 holds; a 32-bit sum, no wrap in practice |
| `internal_value.h:162,183` | `ValueObject::m_refCount` add | relaxed | new reference from an existing one | R1 holds |
| `internal_value.h:166` | `m_refCount` release | `acq_rel`, destroys on 1 | end of a reader's access, destruction | R1 holds |
| `internal_value.h:132` | `use_count()` | acquire | collector reachability, `DropUnshared`, `IsUnheld` | R1 holds |
| `internal_value.cpp:2000,2007,2017` `1183,2590,2593,2607` | `shared_ptr::use_count() == 1` as "alone" | **relaxed** | in-place write, collector `Traverse`/`Clear` | **gap 1** |
| `template_env.cpp:485` | `globalValues.use_count() > 1` | **relaxed** | in-place change of the shared globals map | **gap 1** |
| `borrowed_cells.cpp:101,149,224,233` | plain `m_frozen`, `m_exportedIn`, `m_links` | none; single thread by I1 | live-cell state | I1 broken in one window: **gap 2**; `m_exportedIn` wrap: **gap 3** |
| `template_module.cpp:450,522` | `recursive_mutex` | lock | module calls, S3 | M1 holds |
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
