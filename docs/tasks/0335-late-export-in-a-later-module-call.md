---
status: open
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

Nothing is shared mutably: both kept values are immutable snapshots and no call writes them. The
later kept value is the state at its `keep()` instead of the call's final state, which is the
contract row "from a module call: that call's final state" (docs/api-2.0.md, item 9).

The pull is between that contract and the cost of a module call. Copying at the export when an
earlier snapshot still shares the items fixes it, but `Module/keep` (a call that only hands the
list out while the host still holds the previous one) would then copy 1000 items on every call
(about 1 us to 15 us a call), which D8 was chosen to avoid.

**Proposal.** An export reads its items through a small per-call holder instead of the storage
itself (`ExportListView` holds `const ValuesList&` plus its owner, internal_value.cpp). The
cell keeps the holder of the call that runs; `Unshare` copies into new storage and re-points
the current call's holder at it, so the earlier call's holder keeps the old items. An export
in a call with no earlier snapshot needs no holder, so `Module/keep` keeps its allocations.

**Done when.** `BorrowedCellsKept` has rows for a list and a dict where call 2 runs `keep(l)`
before `l.append(2)` and keeps `[0, 1, 2]` while call 1's value stays `[0, 1]`, ASan and TSan
are clean, and `Module/keep` and `Module/keep_written` do not move in `bench/count.py`.
