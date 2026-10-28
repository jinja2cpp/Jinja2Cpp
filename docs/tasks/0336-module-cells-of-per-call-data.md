---
status: open
priority: medium
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp#GenericListAdapter, src/internal_value.cpp#GenericMapAdapter, test/borrowed_cells_test.cpp]
---
# A template module keeps the cells of generic containers made for one call

**Problem.** A template module's borrowed cells live across its calls (0049 P2), so that a
write to the module's host data in one call is seen by the next. A cell also keeps the value
promoted first (`BorrowedCell::m_source`), so that the identity the registry keys the cell by
is not reused while the cell exists. Both pull against data that lives for one call only.

Since #673 (Codex review comment 4395529333) a module call ends by dropping the cells of
`ValuesList`/`ValuesMap` data that lives only as long as its adapters (a call's argument, a
host callable's result) once nothing but the cell holds its source
(`BorrowedCells::DropUnheld`, tests `BorrowedCellsSingle.ModuleCallFreesTheCellsOf*`). What
is left: a `GenericList`/`GenericMap` the host makes for each call (`consume(l)` with a new
`MakeGenericList(...)` every time) and the call writes. Its identity is the generic
container's accessor, whose owner `GenericList` hides behind a `std::function`, so the cell
cannot tell that nothing else can lend the accessor again, and keeps it: one cell and one
accessor per call until the module goes.

**Proposal.** Give the generic adapters a way to report that their copy of the generic
container is the accessor's only owner (an internal accessor interface the library's own
`MakeGenericList` implements, so no public ABI change), pass it to `CellLink::Adopt` as the
`ValuesListAdapter` does with `AdaptersOwner`, and let `IsUnheld` drop such a cell.

**Design (model-first, 0338 step A; for Ruslan to decide).** The proposal above, with one
condition the model shows is needed: the cell may be dropped only when the generic adapter's copy is
the accessor's **only** owner. If the host keeps its own copy of the `GenericList`, it can lend the
same container to a later call, which must see this call's writes (0049 P2), so the cell stays.
Modelled as `FixGenericOwner` plus a host-copy test in `Unheld`: `module-generic` passes,
`no-generic-owner` (no owner) gives `RegistryBounded` (a cell per call), and
`generic-drop-without-host-test` gives the new invariant `NoLostWrites` (a dropped cell whose data
the host lends again). For the C++: `MakeGenericList` keeps its accessor in a `shared_ptr` that the
library can see through an internal interface (no public ABI change); `IsUnheld` asks for
`use_count() == 1` of it, with the acquire that 0339 requires.

**Done when.** A test that calls a module macro writing a fresh generic list three times
frees the list's data after each call under ASan/LSan, a TSan row with two threads calling
the module is clean, and `Module/keep`, `Module/keep_written` do not move in `bench/count.py`.
