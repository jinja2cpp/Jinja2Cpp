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
python3 tla/check.py                        # every run, about 2.8 hours on 4 cores
python3 tla/check.py --only render mixed    # or in two parts: about 1h45m ...
python3 tla/check.py --only $(python3 tla/check.py --list | awk '$1 != "render" && $1 != "mixed" {print $1}')   # ... and 1h05m
python3 tla/check.py --only f1-nested-export --traces
python3 tla/check.py --list
```

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
| `CellLink` of an adapter (`m_cell`, `m_seenEpoch`, held bit) | `ad[a]`: `cell`, `seen`, `held` |
| `CellCounters` epoch and live | `epoch`, `live` |
| `CellLink::Get`, `Find`, `Link` | `GetR`; an allocation failure in the push of `Link` (c06d969) |
| `BorrowedCell` (`m_storage`, `m_retired`, `m_source`, `m_owner`, `m_exportedIn`, `m_frozen`, `m_links`) | `cl[c]`: `stor`, `retired`, `source`, `owned`, `exp`, `frozen`, `links` |
| A storage: the `ListAdapter`'s accessor object, and the `InternalValueList` it shares | `st[g]`: `acc` (held by the cell only), `alive` (held by the accessor and by exports) |
| `MutableItems`, `Unshare`, `NoteExport` | `WC`, `NeedCopy`, `Export`; a failed copy |
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

What the model abstracts:
- **Values.** A list's content is the number of appends. A copy shares its item adapters, as
  copying an `InternalValueList` does.
- **Memory.** Adapters, cells and storage generations are never reused: a read of a dead one is
  a use-after-free. An owned address (a module call's argument) is reused once no adapter of the
  data there lives, with a new incarnation, so a stale registry key can be detected.
- **Concurrency.** Steps are atomic. A write records its thread and whether that thread held the
  module's lock. The host on another thread may drop or read what it kept only after the call
  returned. `NoCrossThreadWrite` is an ownership rule, stricter than freedom from data races:
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
| open | A module call's export keeps that call's final state (0335) | `ExportKeepsCallState` |

## Results

Bounds: owned addresses `O1` and `O2`, values up to 3, one allocation failure, and two
exports. "Level" is the most states a behaviour has (TLC's BFS depth, the initial state is
level 1), and each GC or DropUnheld step counts as one. `Next` stops at that level, so TLC
explores every behaviour up to it and none past it.

| Run | Scope | Result |
|---|---|---|
| `render` | 1 thread: renders, nested through a callback, level 11 | no error (51.6M states, 82 min) |
| `module` | 1 thread: module calls, nested through a callback, level 12 | no error (1.5M states, 3 min) |
| `mixed` | 1 thread: renders and module calls, level 10 | no error (19.6M states, 23 min) |
| `threads` | 2 threads: renders and the locked module, level 9 | no error (32.0M states, 46 min) |
| `threads-module` | 2 threads: module calls only, level 15 | no error (1.4M states, 3 min) |
| `f1-nested-export` | `module`, the code as merged (no `FixNestedExport`), no allocation failure | **counterexample: `SnapshotStable`** (F1) |
| `no-link-epoch` | c06d969 out (Codex P2) | counterexample: `AliasesAgree` |
| `no-enum-refetch` | f83b89f out (Codex P1) | counterexample: `NoUseAfterFree` |
| `no-enum-refetch-kept` | f83b89f out, no release of exports | counterexample: `NoUseAfterFree` (the loop's accessor freed while the export keeps the items) |
| `no-sort-refetch` | 1e15c2d/89df914 out (Codex P0) | counterexample: `SnapshotStable` |
| `no-call-restore` | 1e15c2d out (Codex P1) | counterexample: `SnapshotStable` |
| `no-nested-global` | 9c91ffc out (Codex P2) | counterexample: `NoForeignCell` |
| `no-mark-order` | 5e37c24 out (verifier r11) | counterexample: `NoForeignCell` |
| `no-sort-mutable-first` | 9c91ffc out (Codex P2) | counterexample: `NoUndefItems` |
| `no-marker-after-copy` | b3993a5 out (Ruslan's P2) | counterexample: `SnapshotStable` |
| `no-read-only-export` | 0331 A1 out (b2.cpp shapes) | counterexample: `NoCrossThreadWrite` (a kept global adapter linked by another thread's render) |
| `no-freeze` | 0331 A2 out (4588321, Ruslan's P2) | counterexample: `SnapshotStable` |
| `no-drop-unheld` | 459d8f7 out (Codex P2) | counterexample: `RegistryBounded` |
| `drop-without-source-check` | `IsUnheld` without the source-owner test | counterexample: `NoUseAfterFree` |
| `drop-without-link-check` | `IsUnheld` without the links test | no error (see below) |
| `no-source-keeps-id` | no `m_source`, no DropUnheld | counterexample: `NoForeignCell` (identity reuse) |
| `open-0335-late-export` | `ExportKeepsCallState` | counterexample (task 0335, expected) |
| `open-0332-kept-list` | `keep([l])` in a module call | counterexample: `SnapshotStable` (task 0332, expected) |
| `open-0332-nested` | a nested item written in a module call | counterexample: `SnapshotStable` (task 0332, expected) |

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
- **The open tasks reproduce as described.** 0335 is a call's own later write missing from its
  export. 0332 is a module's live cell read through a kept template list or a nested item.
  Their runs are expected to fail and are the acceptance checks for those tasks.

## What the results claim

Bounded evidence, not a proof. Each pass run covers every behaviour up to its level within its
bounds, under sequentially consistent interleavings of the steps listed above. The path from
here to a safety argument for the implementation (every ownership and escape path, failure at
each allocation, concurrent reads and destruction, memory order, collection and identifier
reuse, and an inductive or cutoff argument) is task 0338.

## Not modelled

- Dicts. Lists and dicts share the cell code, and the model has lists only.
- `QuietEpoch`/`CellStep`, epoch wrap-around and the memory order of the atomics. The epoch is
  an unbounded counter read atomically.
- The cycle collector's traversal of frozen cells (`Traverse`, `NoteFrozen`). There are no
  cycles: an item is always another identity.
- JSON identities (P5), generic containers made per call (0336), callables' results.
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
| `unlink`, `inherit`, `dieatt` | the link set agrees; a live cell's links change on its writer's thread only; an adapter never goes attached (`NoDanglingLink`) |
| `adopt`, `adoptno`, `adoptundo`, `promote` | a new cell only where the table has none; the live count (`LiveCount`, one-thread traces) |
| `export` (`ExportList`/`ExportMap`) | the mark NoteExport leaves is `CellRules!MarkAfterExport` (`MarkRule`) |
| `unshare`, `copied` | Unshare's outcome is one `CellRules!UnshareOutcomes` allows (`MarkRule`) |
| `write` (`MutableItems`/`MutableDict`) | never a frozen cell (`FrozenUnwritten`); in place only to an unmarked cell or one the running call marked (`MarkRule`); never to a storage a value of an ended call shares (`SnapshotStable`) |
| `begin`, `end` (`BeginCall`, `EndCall`) | call numbers and the call restored (`CallsAgree`); nested calls on the thread holding the lock |
| `drop`, `dropped` (`DropUnheld`) | `CellRules!MayDrop` on the spec's state: owned data, live cell, no link but its source's (`DropRule`) |
| `freeze`, `finish`, `finished` (`Finish`) | the code's links of each cell are the spec's (`LinksAgree`); a global's links detach (`FinishDetaches`), the rest keep the frozen cell; every table cell freezes |
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
and `ModuleSortWhoseKeyHandsOutTheList` fail `MarkRule` at the nested call's export (finding F1,
0337), and with `--no-fix-nested-export` they fail `SnapshotStable` at the outer call's later
write: the defect the model found, seen from the C++. With #689 every trace passes. The
mutants (`--mutants`, `trace_check.py` MUTANTS) change a real trace as a defect would: an adapter
that goes attached, the 0337 mark, a write to a frozen cell, a missed lookup, a foreign read, a
write from another thread, EndCall without its restore, an event the spec lacks. Each fails as
expected.

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
