---
status: in-progress
priority: low
area: parity
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp#ExportListView, test/borrowed_cells_test.cpp, bench/jinja2cpp_bench.cpp]
---
# A module call that hands out a container before it writes it keeps the earlier call's state

**Problem.** Decision D8 of 0331 is copy on write: a value a template module's call hands to the
host keeps that call's final state, and a later call's write copies the items first. The cell
notes only the first call that exported the items (`BorrowedCell::m_exportedIn`). When an
earlier call wrote and exported `l`, and a later call runs `keep(l)` and then `l.append(2)`,
the later export shares the earlier call's items: `NoteExport` keeps the earlier mark, so the
append copies the items and the later call's kept value misses the append. Dicts behave the
same. Found by the Codex review of #673 (comment 4395529329).

For sequential calls, nothing is shared mutably: both kept values are immutable snapshots and no
call writes them. The later kept value is the state at its `keep()` instead of the call's final
state, which is the contract row "from a module call: that call's final state"
(docs/api-2.0.md, item 9). When the later call is nested in the earlier one (a host callback),
the earlier call writes on in place and changes the nested call's kept value after the nested
call ended: that is task 0337, found by the TLA+ model in `tla/`, and the holder proposal below
must keep it copying.

The pull is between that contract and the cost of a module call. Copying at the export when an
earlier snapshot still shares the items fixes it, but `Module/keep` (a call that only hands the
list out while the host still holds the previous one) would then copy 1000 items on every call
(about 1 us to 15 us a call), which D8 was chosen to avoid.

**Proposal.** An export reads its items through a small per-call holder instead of the storage
itself (`ExportListView` holds `const ValuesList&` plus its owner, internal_value.cpp). The
cell keeps the holder of the call that runs; `Unshare` copies into new storage and re-points
the current call's holder at it, so the earlier call's holder keeps the old items. An export
in a call with no earlier snapshot needs no holder, so `Module/keep` keeps its allocations.

The model's `no-call-pin` run (`tla/check.py`, formerly `open-0335-late-export`) gives this task's
counterexample; with the design below modelled, every pass run checks `ExportKeepsCallState`.

**Decision (Ruslan, 2026-10-29).** Call-end pinning as designed below (`FixCallPin`); it also closes 0341. Next: the C++ step.

**Design (model-first, 0338 step A; decided 2026-10-29).** Modelled in `tla/CellProtocol.tla`
as `FixCallPin`; see `tla/README.md`, "Designs of the open tasks".

A module call's exports are *live* while the call runs and *pinned* when it ends. An analogy: the
call hands the host a window onto the module's desk rather than a photocopy; when the call ends,
the window is replaced by a photograph of what the desk then showed, and the next writer to the
desk first takes a fresh sheet (copy on write) instead of drawing over the photographed one.

1. An export in a module call (`ExportList`, `ExportMap`, and the export of a template container,
   `keep([l])`) holds a small per-call export context instead of the cell's items. While the call
   runs, a read through it reads the cell's current storage, on the call's thread, so the export
   sees every later write of the same call, nested calls included. `NoteExport` no longer marks
   anything at export time.
2. `~ModuleCallCollector`, under the module's lock, pins the call's context: for every module cell
   the call's exports reach, found **by identity** in the registry (`GetIdentity()` of the item's
   accessor), it records the cell's storage (sharing the items, a reference count) and sets the
   cell's mark to `SharedExport`. From then on a read through the context uses only the recorded
   storages, never an adapter's link (which the module thread may still change), and an item whose
   identity has no recorded storage reads the host's data.
3. `Unshare` is unchanged: a marked cell copies when the items are not unique, and the pinned
   storage stays as it was. `m_exportedIn` becomes a flag (shared or not): no write compares call
   ids any more, so 0337's sentinel and the call-id wrap of 0341 go away.

What the model checks: `ExportKeepsCallState` (this task) and `SnapshotStable` with nested calls
(0337) both hold in every pass run; `no-call-pin` (the code after 0337) gives this task's
counterexample, `pin-without-marks` and `pin-top-only` show that the mark and the nested pin are
both needed, `call-id-wrap` passes with ids that wrap after 2 while `call-id-wrap-no-pin` fails.

Cost and open choices for the C++ step: one context per call that exports a module cell (a small
allocation; `Module/keep` needs a `bench/count.py` verdict, and the context can live in the
`GenericList`'s own allocation when a call exports one value). Finding the nested cells at the pin
needs a walk of the exported cells' items; it is skipped when the registry holds no cell besides
the exported ones, which is the `Module/keep` case. Pinning every cell of the registry instead is
also safe (it only copies more), and simpler.

**Done when.** `BorrowedCellsKept` has rows for a list and a dict where call 2 runs `keep(l)`
before `l.append(2)` and keeps `[0, 1, 2]` while call 1's value stays `[0, 1]`, ASan and TSan
are clean, and `Module/keep` and `Module/keep_written` do not move in `bench/count.py`.

**Implementation.** `CallExports` in `src/borrowed_cells.h`: `ExportList`/`ExportMap` in the cell's own
module call return a view (`CallListView`, `CallMapView` in `src/internal_value.cpp`) that reads through
the call's exports, and `~ModuleCallCollector` pins them (`BorrowedCells::PinExports`) before
`DropUnheld`. The pin shares the storage's object, so `BorrowedCell::OwnsItemsAlone` sees it. The
nested walk is skipped when the pin already holds every cell of the module. `m_exportedIn` became
the flag `m_shared`; call ids only name calls in the trace and may wrap (0341).
