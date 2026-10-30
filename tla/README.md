# A TLA+ model of the borrowed-cell protocol

`CellProtocol.tla` models how host data, its adapters and the borrowed cells of tasks 0049
(P2) and 0331 live and die. It follows the code as of #673 (merged as a64c314, which includes
459d8f7, DropUnheld). TLC checks it at small bounds. `check.py` runs the checks: the protocol as
it is, the protocol with each #673 fix taken out (each must give a counterexample), and the open
tasks.

The model found one defect the reviews missed, **F1** (task 0337). It also shows that every
fix #673 made is needed: taking any one of them out gives a counterexample.

## Running it

```bash
curl -sSLo tla/tla2tools.jar https://github.com/tlaplus/tlaplus/releases/download/v1.8.0/tla2tools.jar
python3 tla/check.py                        # every run, about 4.8 hours on 4 cores
python3 tla/check.py --only render          # or in three parts: about 2h10m (21 GB of disk) ...
python3 tla/check.py --only mixed threads   # ... about 1h40m ...
python3 tla/check.py --except render mixed threads   # ... and 1h10m
python3 tla/check.py --only f1-nested-export --traces
python3 tla/check.py --list
```

The GitHub Actions workflow `tla-model` (`.github/workflows/tla-model.yml`, task 0350) runs
the same suite in four parallel jobs (`render`, `mixed`, `threads` and the rest) as a routine,
not per pull request: weekly on master when `tla/` changed, and on demand from the Actions tab
(Run workflow, on any branch or tag such as a frozen release candidate, optionally with a list
of runs).
Prefer it to a local run for anything longer than an hour: the job summary has a row per run
and the `tlc-*` artifacts keep each run's `MC.cfg` and `tlc.out`.

The two-thread scopes are reduced by thread symmetry (`SYMMETRY ThreadPerms` over the threads as
model values), which halves them; `--no-symmetry` turns it off. `--except NAME ...` runs every
run but those, `--cache DIR` (or `TLC_CACHE`) skips a run whose spec, model and constants
match an expected verdict recorded there (the workflow keeps one per shard), and `--java-opts` (or `TLC_JAVA_OPTS`) passes JVM options to TLC.

Java 11 or later is needed. `--set Name=Value` overrides a constant, for example
`--set MaxLevel=14`. `--timeout SECONDS` limits each run; a run stopped by it fails as
inconclusive, so a pass is always a completed search of its bounded space. Each run writes `MC.cfg` and `tlc.out` into a temporary directory; the
last line of the output names it. The TLA+ Toolbox or the VS Code TLA+ extension can open
`CellProtocol.tla` with such an `MC.cfg`.

## What is modelled

An analogy for reading the spec: the host's data is a library book that nobody may write in.
The first time a template writes to the book, the render photocopies it into a cell and keeps
the copy at a desk (the registry), filed under the book's shelf mark (its identity). Every
reader of the same book is then sent to that copy. When the render ends, the desk closes:
copies someone took home stay readable but are sealed (frozen), and the rest go to the shredder
(GC). A template module keeps its desk open across calls, so a copy handed out in one call must
not change in a later one. That rule is D8's copy on write.

| Code | Model |
|---|---|
| `CellLink` of an adapter (`m_cell`, `m_seenEpoch`, held bit, owns bit) | `ad[a]`: `cell`, `seen`, `held`; `OwningLink(a)` |
| `CellCounters` epoch and live | `epoch`, `live` |
| `CellLink::Get`, `Find`, `Link` | `GetR`; an allocation failure in the push of `Link` (c06d969) |
| `BorrowedCell` (`m_storage`, `m_retired`, `m_source`, `m_owner`, `m_shared`, `m_frozen`, `m_links`) | `cl[c]`: `stor`, `retired`, `source`, `owned`, `exp`, `frozen`, `links` |
| A storage: the `ListAdapter`'s accessor object, and the `InternalValueList` it shares | `st[g]`: `acc` (held by the cell only), `alive` (held by the accessor and by exports) |
| `MutableItems`, `Unshare`, `ExportList`/`ExportMap` | `WC`, `NeedCopy`, `Export`; a failed copy |
| `CellLink::Adopt` | `Adopt`: a copy of the data with one nested item adapter (`P` holds `C`) |
| `BorrowedCells` of a render and of a module, `BeginCall`/`EndCall` | `rg[r]`: `tbl`, `call`, `last` |
| `BorrowedCells::Finish` | `FinishSources`, then `FinishFreeze` (sources first, then freeze) |
| `ModuleCallCollector`, `DropUnheld`/`IsUnheld` (459d8f7) | `EndModCall`, `DropUnheld`, `Unheld`; `DropRule` |
| `TemplateModuleState::Call` under its recursive mutex | `StartModCall`, `lock` |
| `GetGlobalsSnapshot`, `RenderWorkspace::GlobalAttached` | `StartRender`, `gsnap`, `gatt`; a failed conversion |
| `ExportListView` and its owner (A1); `CreateGenericList` copies the accessor | an export of kind `gen` holds a storage's items, not its accessor |
| `keep([l])`: a template list holding an adapter | an export of kind `view` |
| `CellEnumerator` / `StepEnumerator` across a callback | `EnumStart`, `EnumStep` |
| `ListSort` with its `Restore` guard across key calls | `SortStart`, `SortEnd` |
| reference counts and destructors | `GC`: reachability from the roots, run right after the step that dropped a reference |
| a module namespace holding an argument | `ns` |
| `CallExports`, pinned in `~ModuleCallCollector` (0335, 0332); its views `CallListView`, `CallMapView` | an export of kind `live`; `PinEx` in `EndModCall` |
| A link holds a reference to its cell unless it is a global's (`CellLink::OwnsBit`; 0332, 0340) | `OwningLink`, `FixModuleLinksOwn`, `FixLinksOwn` |
| 0340 option (a), not in the C++: a render's live cell holds its linked adapters until `Finish` | `Succ` of a cell, `FixPinOnAttach` |
| `l.pop()` of a nested list | op `drop` |
| a generic container (`MakeGenericList`) as a module call's argument, a copy the host keeps (0336) | `ad[a].generic`, `hkeep`, `HostLetsGo` |
| `NextEpoch`, `BeginCall` wrapping past 32 bits (0342, 0341) | `NextEp`, `NextCall` with `MaxEpoch`, `MaxCallId` |

What the model abstracts:
- **Values.** A list's content is the number of appends. A copy shares its item adapters, as
  copying an `InternalValueList` does.
- **Memory.** Adapters, cells and storage generations are never reused: a read of a dead one is
  a use-after-free. An owned address (a module call's argument) is reused once no adapter of the
  data there lives, with a new incarnation, so a stale registry key can be detected.
- **Concurrency.** Steps are atomic. A write records its thread and whether that thread held the
  module's lock. The host on another thread may drop or read what it kept only after the call
  returned; with `HostHandoff` (0340) it may drop it on another thread at any time. `NoCrossThreadWrite` is an ownership rule, stricter than freedom from data races:
  an object written by two threads is a violation even when the writes are ordered, unless both
  held the module's lock.
- **Host data.** It is constant. No step writes it, as in the code, so the first invariant holds
  by construction. The model checks the stronger statement that nothing a host keeps changes.

## Invariants

| # | Invariant | Name |
|---|---|---|
| 1 | The host's data is never written | `HostNeverWritten` (structural) |
| 2 | A snapshot the host keeps never changes after its call ends | `SnapshotStable` |
| 3 | No two threads write the same cell, adapter or storage, unless both hold the module's lock | `NoCrossThreadWrite` |
| 4 | Nothing reads freed storage, and no live adapter points at a freed cell | `NoUseAfterFree` |
| 5 | A lookup never returns a cell of other data (identity reuse) or a live cell of another render | `NoForeignCell` |
| 6 | Between module calls, the registry holds no cell of a finished call's own data | `RegistryBounded` |
| + | An alias never reads the host's data while its render has a cell of it | `AliasesAgree` |
| + | No moved-from item is stored | `NoUndefItems` |
| + | `live` equals the number of listed cells | `LiveCount` |
| 7 | A module call's export keeps that call's final state (0335) | `ExportKeepsCallState` |
| 8 | No cell is dropped while its data can be lent to a later call (0336) | `NoLostWrites` |

## Results

Bounds: owned addresses `O1` and `O2`, values up to 3, one allocation failure, and two
exports. "Level" is the most states a behaviour has (TLC's BFS depth, the initial state is
level 1), and each GC or DropUnheld step counts as one. `Next` stops at that level, so TLC
explores every behaviour up to it and none past it.

| Run | Scope | Result |
|---|---|---|
| `render` | 1 thread: renders, nested through a callback, level 11 | no error (80.6M states, 130 min; about 21 GB of TLC state on disk) |
| `module` | 1 thread: module calls, nested through a callback, level 12 | no error (1.4M states, 3 min) |
| `mixed` | 1 thread: renders and module calls, level 10 | no error (28.6M states, 35 min) |
| `threads` | 2 threads: renders and the locked module, level 9 | no error (42.6M states, 61 min) |
| `threads-module` | 2 threads: module calls only, level 15 | no error (1.3M states, 4 min) |
| `threads-module-kept` | 2 threads: module calls, `keep([x])`, hand-off, level 16 | no error (4.2M states, 9 min) |
| `module-0332` | 1 thread: module calls with `keep([x])`, nested writes and `drop`, level 11 | no error (3.1M states, 7 min) |
| `render-handoff` | 2 threads: renders, kept lists dropped on the other thread mid-render, level 10, owning links (`FixLinksOwn`, the C++) | no error (5.9M states, 52 min) |
| `render-handoff-pin-on-attach` | `render-handoff` with 0340 option (a) instead (`FixPinOnAttach`, no `FixLinksOwn`) | no error (5.9M states, 34 min) |
| `call-id-wrap` | `module` with call ids wrapping after 2 | no error (1.4M states, 3 min) |
| `module-generic` | 1 thread: module calls taking generic containers the host may keep | no error (5.2M states, 11 min) |
| `f1-nested-export` | `module`, the code before 0337 (no `FixNestedExport`, no `FixCallPin`), no allocation failure | **counterexample: `SnapshotStable`** (F1) |
| `no-link-epoch` | c06d969 out (Codex P2) | counterexample: `AliasesAgree` |
| `no-enum-refetch` | f83b89f out (Codex P1) | counterexample: `NoUseAfterFree` |
| `no-enum-refetch-kept` | f83b89f out, no release of exports | counterexample: `NoUseAfterFree` (the loop's accessor freed while the export keeps the items) |
| `no-sort-refetch` | 1e15c2d/89df914 out (Codex P0) | counterexample: `SnapshotStable` |
| `no-call-restore` | 1e15c2d out (Codex P1), without `FixCallPin` | counterexample: `SnapshotStable` (with `FixCallPin` no error: no write compares call ids) |
| `no-nested-global` | 9c91ffc out (Codex P2) | counterexample: `NoForeignCell` |
| `no-mark-order` | 5e37c24 out (verifier r11) | counterexample: `NoForeignCell` |
| `no-sort-mutable-first` | 9c91ffc out (Codex P2) | counterexample: `NoUndefItems` |
| `no-marker-after-copy` | b3993a5 out (Ruslan's P2) | counterexample: `SnapshotStable` |
| `no-read-only-export` | 0331 A1 out (b2.cpp shapes) | counterexample: `NoCrossThreadWrite` (a kept global adapter linked by another thread's render) |
| `no-freeze` | 0331 A2 out (4588321, Ruslan's P2) | counterexample: `SnapshotStable` |
| `no-drop-unheld` | 459d8f7 out (Codex P2) | counterexample: `RegistryBounded` |
| `drop-without-source-check` | `IsUnheld` without the source-owner test, without `FixModuleLinksOwn` and `FixLinksOwn` | counterexample: `NoUseAfterFree` (with owning links no error: the namespace's link keeps the cell) |
| `drop-without-link-check` | `IsUnheld` without the links test | no error (see below) |
| `no-source-keeps-id` | no `m_source`, no DropUnheld | counterexample: `NoForeignCell` (identity reuse) |
| `no-call-pin` | the code after 0337 (no `FixCallPin`) | counterexample: `ExportKeepsCallState` (0335) |
| `no-call-pin-kept-list` | no `FixCallPin`, `keep([l])` in a module call | counterexample: `SnapshotStable` (0332) |
| `no-call-pin-nested` | no `FixCallPin`, a nested item written in a module call | counterexample: `SnapshotStable` (0332) |
| `pin-without-marks` | the pin marks no cell shared | counterexample: `SnapshotStable` |
| `pin-top-only` | the pin leaves the nested item's cell live | counterexample: `SnapshotStable` |
| `module-links-unowned` | no `FixModuleLinksOwn` (nor `FixLinksOwn`): a later call's adapter kept, dropped on the other thread | counterexample: `NoCrossThreadWrite` (0332) |
| `render-links-unowned` | no `FixLinksOwn` (nor `FixPinOnAttach`), hand-off | counterexample: `NoCrossThreadWrite` (0340) |
| `call-id-wrap-no-pin` | call ids wrap after 2, no `FixCallPin` | counterexample: `SnapshotStable` (0341) |
| `no-generic-owner` | no `FixGenericOwner` | counterexample: `RegistryBounded` (0336) |
| `generic-drop-without-host-test` | `IsUnheld` without the host-copy test | counterexample: `NoLostWrites` (0336) |
| `open-0342-epoch-wrap` | `render` with the epoch wrapping after 2 | counterexample: `AliasesAgree` (task 0342, expected) |

Every counterexample takes 1 to 80 seconds; `check.py --traces` prints its steps.

**What the scopes cover.** Every pass run explored its whole bounded space. Their levels are 5 to
7 template steps, plus the GC steps between them, and they are lower than the levels of the scopes
the mutants use, which only need to reach a counterexample. The deep counterexamples (the loop and sort rows) need about 13
template steps, so their runs use a scope with only module calls, one cell and the steps a
borrower needs (`DEEP` in `check.py`).

### F1: a nested module call's kept value is written by the outer call (task 0337)

Trace (`f1-nested-export`, no allocation failure needed):

1. Call 1 runs `l.append(1)` then `keep(l)`. `NoteExport` sets `m_exportedIn = 1`.
2. Call 1 calls a host callable, which calls the module again. Call 2 runs `keep(l)` and does
   not write. `NoteExport` keeps the mark 1, so call 2's kept value shares call 1's items.
3. Call 2 returns, and `EndCall` restores call 1's id.
4. Call 1 runs `l.append(2)`. `Unshare` sees `m_exportedIn == m_call` and writes in place, so call
   2's kept value changes after call 2 ended. A host thread reading it races with call 1.

0335 has the same root, one mark per cell, but its text said "nothing is shared mutably". That
holds for sequential calls only. The fix (task 0337), modelled as `FixNestedExport`: when
`NoteExport` meets another call's mark, it sets `BorrowedCell::SharedExport`, which no call id
equals (`BeginCall` skips it), so every later write goes through the uniqueness test. The pass
runs use it, and `f1-nested-export` keeps the code before the fix. The tests are
`BorrowedCellsKept.ModuleNestedKeepOfTheOuterCallsExport` and the dict and TSan rows after it.

### Other observations

- **A module call's teardown runs under the module's lock** (Codex review of #688).
  `TemplateModuleState::Call` holds its mutex while the arguments die and
  `ModuleCallCollector` is torn down. The model first recorded the GC step after `EndModCall` as
  a write without the lock, which gave a spurious `NoCrossThreadWrite` for two sequential calls
  on two threads at level 10. That GC step now writes as the call did (`dirtyBy`), and
  `threads-module` checks the two-thread module case to level 15.
- **A storage's accessor and its items have separate lifetimes** (Ruslan's review of #688).
  `CreateGenericList` copies the storage's `ListAdapter` accessor and shares its
  `InternalValueList`, so an export keeps the items but not the accessor object in `m_storage`.
  The loop before f83b89f held that accessor's enumerator. The first version of the model had
  one lifetime for both. There, an outer loop over a kept export never read freed memory
  (`no-enum-refetch-kept` explored its whole space without an error). The model now has `acc`
  apart from `alive`, and that run gives the counterexample: two nested copies retire the
  loop's storage while the host still holds its items.
- **`IsUnheld`'s links test never decides in the model** (`drop-without-link-check` passes).
  Owned data has one adapter per call here. The test guards the `LifetimeAnchor` adapters of
  copied items, which the model does not have, so the run neither confirms nor refutes it.
- **The source-owner test of `IsUnheld` is needed.** Without it, a module call's argument
  stored in the namespace loses its cell while the namespace's adapter is still attached
  (`drop-without-source-check`, a use-after-free).
- **The open tasks reproduced as described.** 0335 is a call's own later write missing from its
  export. 0332 is a module's live cell read through a kept template list or a nested item. Their
  runs are now the mutants of the designs below (`no-call-pin*`).
- **Two #673 fixes become redundant under the designs.** `FixCallRestore` guards a comparison of
  call ids that `FixCallPin` removes, and the source-owner test of `IsUnheld` guards a
  use-after-free that owning module links remove. Their mutants run without the design that
  covers them, so they still show the model sees the defect in #673's code. The C++ may keep both
  (cheap, and the source test still keeps aliases of a namespace value on one cell).

## Designs of the open tasks (0338 step A)

The protocol is designed here first and in C++ after (task 0338, "Proposed approach" 1). Each
design is a `Fix` constant, on by default, with mutant runs that take it out or weaken it. The task
files carry the proposals for decision: 0335, 0332, 0336; 0340 and 0341 are answered here.

**Pin a module call's exports at its end (`FixCallPin`: 0335, 0332, 0337, 0341).** One mark per
cell cannot say which call's value shares the items, and that gave 0335 (a later call's export
misses its own later write), F1/0337 (a nested call's export changes after it ended) and 0341 (a
wrapped call id equals a stale mark). The resolution moves the decision from the export to the
call's end, where it is known what the call's value must show. While a call runs, its exports read
the module's cells (`live`): the host on the call's thread sees the call's writes, including those
of nested calls. `EndModCall` pins them: each export records the storage of every module cell it
reaches, found by identity in the registry, and marks those cells shared, so the next write copies.
A pinned export never reads an adapter's link, which the module thread may still change. No write
compares call ids any more, so 0337's sentinel is subsumed and the id may wrap
(`call-id-wrap` passes with `MaxCallId = 2`). The pin covers `keep([l])` and nested items, the
two shapes of 0332, which are now in the contract (`module-0332`).

**A link to a module's cell owns the cell (`FixModuleLinksOwn`: 0332).** A copy of a list shares its
item adapters, so the host's value can hold the very adapter a module cell links. If the host drops
the last reference on its thread, `Unlink` edits the live cell's `m_links` there. With an owning
link, as a frozen cell's links already are, the release is a reference count. The model reaches
the case only through a later call's adapter (the first adapter is the cell's `m_source`, which the
cell keeps alive): `module-links-unowned`.

**Every link but a global's owns its cell (`FixLinksOwn`: 0340).** A render's frames go before
`Finish`, so a kept container can be the last holder of an attached adapter in that window, and a
host thread that drops it unlinks from the live cell (`render-links-unowned`, with `HostHandoff`).
The decision was option (a) of 0340, the registry holding the adapters (`FixPinOnAttach`). The C++
makes the render's links owning instead, the rule `FixModuleLinksOwn` already sets for a module's
cells: a link to a cell in an `ItemAdapters` block has no object of its own the registry could
hold (the block owns it), while the link always knows its cell. A global's link stays listed and
non-owning, since `Finish` must find it to detach it; the thread's snapshot holds a global's
adapter, so a host's drop is never its last reference, except for a global's nested item whose
block of item adapters the global's adapter let go of (task 0346; the model has no blocks). Both
closings are runs:
`render-handoff` (the C++) and `render-handoff-pin-on-attach` (option a). Option (b), an export of
a detached copy, would lose the frozen cell that a kept value reads after the render (0331 A2)
unless renders also pinned by identity.

**Drop a generic container's cell only when the host keeps no copy (`FixGenericOwner`: 0336).**
`no-generic-owner` keeps a cell per call (`RegistryBounded`); a drop without the host-copy test
loses the writes a later call with the same container must see (`generic-drop-without-host-test`,
`NoLostWrites`).

**Still open.** 0342, the epoch wrap, is a measurement decision (stored epoch width); the model now
reproduces it (`open-0342-epoch-wrap`, `MaxEpoch = 2`). 0339 (a relaxed `use_count` in the
uniqueness test, now an acquire) is memory order, outside a sequentially consistent model; the
designs above keep the uniqueness test, so they inherit its fix.

## What the results claim

Bounded evidence, not a proof. Each pass run covers every behaviour up to its level within its
bounds, under sequentially consistent interleavings of the steps listed above. The path from
here to a safety argument for the implementation (every ownership and escape path, failure at
each allocation, concurrent reads and destruction, memory order, collection and identifier
reuse, and an inductive or cutoff argument) is task 0338.

## Not modelled

- Dicts. Lists and dicts share the cell code, and the model has lists only.
- `QuietEpoch`/`CellStep` and the memory order of the atomics (argued in
  `docs/design/borrowed-cells-memory-order.md`). The epoch and the call id wrap
  only in the runs that bound them (`MaxEpoch`, `MaxCallId`).
- The cycle collector's traversal of frozen cells (`Traverse`, `NoteFrozen`). There are no
  cycles: an item is always another identity.
- JSON identities (P5) and callables' results. Generic containers (0336) are modelled as module
  call arguments only.
- A module destroyed during its own call, and the freeze at module death.
- Python parity of values. The model checks lifetimes and isolation, not output.

## Trace validation (task 0338, step B)

The model checks the protocol; trace validation checks that the C++ follows it. A build
configured with `-DJINJA2CPP_CELL_TRACE=ON` (a Debug or test build; off by default, and every
hook compiles to nothing without it) writes the protocol's events to the file the environment
variable `JINJA2CPP_CELL_TRACE` names, one JSON object a line (`src/cell_trace.h`).
`check.py --trace` runs tests one per process, turns each log into the module `TraceLog` and
runs TLC on `CellTrace.tla`, which reads the log one event at a time and keeps the protocol's
state: cells, the links of the adapters, the registries with their tables and running calls,
and what the values a module call handed out share.

```bash
cmake -S . -B build-trace -G Ninja -DCMAKE_BUILD_TYPE=Debug -DJINJA2CPP_CELL_TRACE=ON
cmake --build build-trace --target jinja2cpp_tests
python3 tla/check.py --trace --tests build-trace/jinja2cpp_tests            # cell and module tests, 1-2 min
python3 tla/check.py --trace --tests build-trace/jinja2cpp_tests --mutants  # and the checker's own mutants
python3 tla/check.py --trace --tests build-trace/jinja2cpp_tests --filter '*' --per-suite   # every test
python3 tla/check.py --trace --logs some.ndjson                              # a log written elsewhere
```

A log is whole only when the process that wrote it exited normally: the logger aborts the
process when it cannot open or write the file, since the file itself cannot record that events
were lost. `--tests` fails a test that did not exit 0; with `--logs`, check the writer's exit
status yourself.

A trace fails in one of two ways. **No step takes an event** (TLC's deadlock): the code did
something the spec has no transition for. **An invariant fails**: the step exists, but the
code's decision or state differs from the rules, or breaks a property of the model as far as a
trace can show it. The events and what the spec checks at each:

| Event (C++) | Checked |
|---|---|
| `skip` (`CellLink::MayHaveCell` false) | the epoch is the one the adapter saw at its last lookup (`EpochAgrees`); the running registry has no cell of its data (`AliasesAgree`) |
| `find`, `link`, `linkfail` (`Find`, `Link`) | the lookup finds the registry's cell of that identity and kind (`TableAgrees`); a link only to a live, unfrozen cell, by its registry's writer (`NoCrossThreadWrite`) |
| `hit` (an attached adapter's read) | the adapter is attached to that cell (`LinksAgree`); the cell is alive and, unless frozen, of the running registry (`NoForeignCell`) |
| `unlink`, `inherit`, `dieatt` | the link set agrees, and whether the link owns its cell (`CellRules!LinkOwns`); an owning link lets go on any thread, a global's link changes a live cell's list on its writer's thread only; an adapter never goes attached (`NoDanglingLink`) |
| `adopt`, `adoptno`, `adoptundo`, `promote` | a new cell only where the table has none; the live count (`LiveCount`, one-thread traces) |
| `export` (`ExportList`/`ExportMap`) | a value reads through the call's `CallExports` exactly in the cell's own module call (`CellRules!ExportLive`, `MarkRule`); it marks nothing |
| `pin` (`CallExports::Pin`) | a live table cell of the registry whose call ends; the cell is marked and its storage is what the host's values share from now on (`exr`) |
| `unshare`, `copied` | Unshare's outcome is one `CellRules!UnshareOutcomes` allows (`MarkRule`) |
| `write` (`MutableItems`/`MutableDict`) | never a frozen cell (`FrozenUnwritten`); in place only to an unmarked cell (`MarkRule`); never to a storage a value of an ended call shares (`SnapshotStable`) |
| `begin`, `end` (`BeginCall`, `EndCall`) | call numbers and the call restored (`CallsAgree`); nested calls on the thread holding the lock |
| `drop`, `dropped` (`DropUnheld`) | `CellRules!MayDrop` on the spec's state: owned data, live cell, no link but its source's (`DropRule`); the code's list is the global's links |
| `freeze`, `finish`, `finished` (`Finish`) | the code's list of each cell is the spec's global links, every other link owns (`LinksAgree`); a global's links detach (`FinishDetaches`), the rest keep the frozen cell; every table cell freezes |
| `free` (`~BorrowedCell`) | no table lists the cell and no adapter is attached to it (`NoUseAfterFree`) |
| `regnew`, `regfree` | a registry goes with an empty table and no running call |

What only the host knows is an input that the log supplies, and the spec checks only that the
choice was one the rules allow: reference counts (whether Unshare finds the items owned alone,
whether DropUnheld's counts hold), and which objects die when. Objects are the log's numbers: a
cell, a storage and a registry get a new number when made; a link and an identity are named by
their address.

`CellRules.tla` holds the step rules as operators of plain values. `CellTrace.tla` uses them;
`CellProtocol.tla` states the same rules inline (`Export`, `WC`, `FinishFreeze`, `Unheld`). Once
the contract work of 0338 step A freezes the rules, `CellProtocol.tla` should use the
`CellRules` operators too, so that both specs share one statement of each rule.

**Results.** On master before #689, the traces of `BorrowedCellsKept.ModuleNestedKeepOfTheOuterCallsExport`
and `ModuleSortWhoseKeyHandsOutTheList` failed `MarkRule` at the nested call's export (finding F1,
0337), and without that fix they failed `SnapshotStable` at the outer call's later write: the
defect the model found, seen from the C++. Since 0335/0332 an export in a module call marks
nothing; the call's end pins the cells (`pin`) and the next write copies, which the trace checks
with `MarkRule` and `SnapshotStable`. Every trace passes (81 with `--mutants`). The mutants
(`--mutants`, `trace_check.py` MUTANTS) change a real trace as a defect would: an adapter that
goes attached, a pin that leaves its cell unmarked, a write to a frozen cell, a missed lookup, a
foreign read, a write from another thread, EndCall without its restore, an event the spec lacks.
Each fails as expected. A test's log must stay small for TLC: the 0340 drop row runs 20 renders
instead of 2000 under `JINJA2CPP_CELL_TRACE` (400k events ran out of TLC's heap).

**Not traced.** The block-level skips (`QuietEpoch`, `ItemAdapters` slots released without a
call, `CellStep` steps after the first): they read no cell by construction and log nothing. The
order of events across threads is the order the trace's mutex took them, which agrees with the
code's order for state one thread or the module's lock owns; the counters' memory order is step
C's (task 0339 onwards). The epoch and call numbers are logged as the 32-bit values they are, so
a wrap shows as `CallsAgree` or `EpochAgrees`; the tests do not reach one.

## Changing the protocol

Change the spec together with the code it mirrors (the table above). Keep the mutant runs: a
mutant that stops failing means the model no longer sees that defect. A new fix gets a
`FixXxx` constant, defaulting to the fixed behaviour, and a mutant row in `check.py`.
