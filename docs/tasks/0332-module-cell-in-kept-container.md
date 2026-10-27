---
status: open
priority: low
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, test/borrowed_cells_test.cpp]
---
# A module's live cell read through a container the host keeps

**Problem.** A template module's borrowed cells live across its calls (docs/tasks/0049 P2).
Since 0331 D8 a container a call hands to the host directly (`keep(l)` after `l.append(x)`)
is copied on a later call's write, so the host keeps that call's final state. A container
the call makes that holds the adapter (`keep([l])`) does not get that protection: the host
keeps the template's list, whose item is `l`'s adapter, attached to the module's live cell.
A later call's write shows through it (`[[0, 1, 2]]` where the first call ended with
`[[0, 1]]`), and reading it on another thread while a module call writes the cell is a data
race, which Ruslan's rule (no mutable state shared between renders, threads and the host)
excludes. The values are freed correctly; the cell freezes when the module goes.

Pinned by `BorrowedCellsKept.ModuleCellInsideAKeptList` (current behaviour, with the target in
its comment).

**Options.**
- Freeze the module's cells at the end of every call and give the next call that writes a
  fresh cell copied from the frozen one, re-linking the module's own adapters (option 3 of
  the D8 brainstorm). Costs an O(n) copy for every call that writes, unless the frozen cell is
  found unshared.
- Note, at the export of a template container during a module call, that the call handed out
  values; at the next call's first write to any cell, copy every cell whose links include one
  held from outside the module's context (needs a way to tell those links apart).

**Done when.** The test row expects `[[0, 1]]` and a TSan row (two threads calling the module,
one reading the kept list) is clean, with `Module/keep` unchanged in `bench/count.py`.
