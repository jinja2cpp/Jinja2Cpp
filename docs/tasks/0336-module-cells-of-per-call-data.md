---
status: in-progress
priority: medium
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.h, src/internal_value.cpp#GenericListAdapter, src/internal_value.cpp#GenericMapAdapter, include/jinja2cpp/generic_list.h, include/jinja2cpp/value.h, include/jinja2cpp/make_generic_list.h, include/jinja2cpp/reflected_value.h, include/jinja2cpp/json_adapter.h, test/borrowed_cells_test.cpp]
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

**Decision (Ruslan, 2026-10-29).** The generic-owner rule with the host-copy test (`FixGenericOwner`). Next: the C++ step.

**Design (model-first, 0338 step A; decided 2026-10-29).** The proposal above, with one
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

**Implementation.** The generic adapters pass their copy's owner to `CellLink::Adopt` (not for a
global's data), and `IsUnheld` asks the source adapter `MayBeLentAgain()`. A generic container is
the accessor's only owner when the accessor derives from `detail::OwnedAccessor` (an
`enable_shared_from_this` base the library's own accessors take: `MakeGenericList`, `Reflect` of a
container or struct, the JSON bindings), the container's `shared_ptr` shares the control block that
owns it (not an aliasing pointer, as the 1.x callable form or a nested item's `WithOwner` makes),
and `IsSoleOwner` (0339's acquire) holds for that pointer. The library reads the pointer through
`detail::GenericAccessorAccess`, a friend of `GenericList` and `GenericMap`: no exported function or
class layout of the library changes (the library adds the typeinfo of `detail::OwnedAccessor`); the
header-only accessor templates gain the base (16 bytes). Any other accessor keeps its cell until the
module goes, as before. Limit, filed as 0365: a fresh generic container nested in a `ValuesList`
argument is lent by reference (no owner), so its cell still stays per call. The TLA+ model already had the
rule (`FixGenericOwner` with the host-copy test in `Unheld`); the trace's `adopt` now reports
`owned` for a generic container's cell, which `MayDrop` reads.

