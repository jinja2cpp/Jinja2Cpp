---
status: in-progress
priority: high
area: robustness
depends: [0340]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/711
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp#ItemAdapters, src/internal_value.cpp#LendHeldItem, test/borrowed_cells_globals_test.cpp, tla/CellProtocol.tla, tla/CellTrace.tla, tla/CellRules.tla]
---
# A global's item adapter in a block the global let go of, dropped on another thread

**Problem.** Since the fix of 0340 every link owns its cell except a global's (`CellLink::OwnsBit`):
a global's link stays in the live cell's `m_links`, because `BorrowedCells::Finish` must find it to
detach it, as the thread's snapshot outlives the render (0211). Letting go of such a link edits that
list, so it must happen on the render's thread before `Finish`. For the global's own adapter it
does: the thread's snapshot holds it. For a nested item of a global it need not. The item adapters
live in blocks of 256 (`ItemAdapters<true>`), and the global's adapter keeps only the block it read
last (`ValuesListAdapter::LendNested`). So: a template writes `g[300]` (a list item of a global with
more than 256 items; its link is a global's, listed), hands `[g[300]]` to the host, then reads `g[0]`,
and the global's adapter lets go of the block of `g[300]`. The host's value is now the block's only
holder. A host thread that drops it while the render runs calls `CellLink::Unlink` on the listed link
and edits `m_links` of the live cell while the render thread may link another global's adapter to
it or `Finish` walks it: the race of 0340, for this shape only.

Found by reading, while implementing 0340 (PR for 0335/0332/0340/0341); not reproduced yet.

**Proposal.** Either (a) a held link takes a reference to the object that owns its adapter (the block
for an item, the adapter itself otherwise) and the registry releases those references in `Finish`,
0340's option (a) for the listed links only; it needs a back pointer from a held item adapter to its
block (the held adapters are a type of their own, 0318, so the other blocks do not grow); or (b) a
global's item adapter that is written (`Adopt`, `Link` with `held`) keeps its block in the global's
adapter until `Finish` (a small list in the adapter, cleared at the detach).

**Done when.** A TSan row (a global of 300 lists, `g[299].append(1)`, `keep([g[299]])`, `g[0]` read,
the kept value dropped on another thread during the render, 2000 renders) is clean, and
`Render/for_filter_if` and `Render/chat_llama` do not move in `bench/count.py`.

**Found on the way.** The shape is wider than the block of the description, and TSan reproduces it
on master (`test/borrowed_cells_globals_test.cpp`: data races and a heap-use-after-free of the
cell's `m_links` buffer, which the render's `push_back` reallocates while the host's `Unlink`
writes it). `g[i]` of a global (`GetItem`) and a map's item (`g.m`) are not lent from a block at
all: `LendHeldItem` makes a new adapter at each read, which no snapshot keeps, and its link was
listed as well. A template that writes through `x = g.m` in a macro and hands `[x]` to the host
leaves the host the last reference once the macro's frame goes. The block shape is a loop over the
global (`for x in g`, `HostCurrentItem`), which reads past the block of the kept item; there TSan
also reports `Finish` writing the link of an adapter the host holds and frees. A module call never
lists a global's link: a template module converts its globals by value (`TemplateModuleState`).
Priority raised to high: memory corruption reachable from template text when the host drops kept
values on another thread, which 0340 made part of the contract.

**Resolution.** The model first: `tla/CellProtocol.tla` gets `FixSnapshotLinks`, an adapter's
`snap` (the snapshot reaches it) and `blk` (the item adapters its block holds), and the actions
`LendGItem`, `Evict` and `Forget`; `global-items-listed` (the code before) gives a
`NoCrossThreadWrite` counterexample, `global-items-handoff` and `render-global-items` pass
(tla/README.md, "A global's link is listed only while the snapshot reaches its adapter"). Then the
C++: a link is listed only if its adapter is a global's that the thread's snapshot reaches
(`CellLink::SnapshotBit`, set by `LendGlobal` on a global's own adapter and by `LendNested` on the
items of such an adapter's block); every other link owns its cell. When the global's adapter lets
go of a block, `ItemAdapters::LeaveSnapshot` walks it (and the blocks its items hold), and each
listed link leaves its cell's list and takes a reference, on the render's thread, which still holds
the block. Neither option of the description was taken: (a) would keep every adapter of `g[i]` read
in a loop alive until `Finish`, and (b) does not cover the adapters no block holds. The trace
validation has the new event `leave`.
