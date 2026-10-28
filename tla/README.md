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
python3 tla/check.py                        # every run, about 2.7 hours on 4 cores
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
| `mixed` | 1 thread: renders and module calls, level 10 | no error (19.5M states, 25 min) |
| `threads` | 2 threads: renders and the locked module, level 9 | no error (32.0M states, 45 min) |
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
holds for sequential calls only. The proposed fix, modelled as `FixNestedExport`: when
`NoteExport` meets another call's mark, it sets a sentinel no call id equals, so every later
write goes through the uniqueness test. The pass runs use it. The pinned test is
`BorrowedCellsKept.ModuleNestedKeepOfTheOuterCallsExport`.

### Other observations

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

## Not modelled

- Dicts. Lists and dicts share the cell code, and the model has lists only.
- `QuietEpoch`/`CellStep`, epoch wrap-around and the memory order of the atomics. The epoch is
  an unbounded counter read atomically.
- The cycle collector's traversal of frozen cells (`Traverse`, `NoteFrozen`). There are no
  cycles: an item is always another identity.
- JSON identities (P5), generic containers made per call (0336), callables' results.
- A module destroyed during its own call, and the freeze at module death.
- Python parity of values. The model checks lifetimes and isolation, not output.

## Changing the protocol

Change the spec together with the code it mirrors (the table above). Keep the mutant runs: a
mutant that stops failing means the model no longer sees that defect. A new fix gets a
`FixXxx` constant, defaulting to the fixed behaviour, and a mutant row in `check.py`.
