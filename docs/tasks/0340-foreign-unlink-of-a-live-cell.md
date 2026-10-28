---
status: open
priority: medium
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp, test/borrowed_cells_test.cpp, tla/CellProtocol.tla]
---
# A host thread dropping a kept container unlinks an adapter from a live cell

**Problem.** A `CellLink` belongs to an adapter *object*. Copying an `InternalValue` shares the
object, so a container the host keeps can hold the adapter that is attached to a live cell
(`keep([l])`, the case of 0332). The render's frames hold the adapter until they are destroyed, which
is before `CycleCollector::Finish` freezes the registry (the collector is made first, so it ends
last). In that window the kept container is the only holder. A host thread that drops it runs
`~OwnedAdapter` and `CellLink::Unlink` (`borrowed_cells.cpp:219`) on a live cell:

1. It reads the plain `bool m_frozen` while `Finish` writes it (`borrowed_cells.cpp:378`). That is a
   data race.
2. When it reads `false` it edits `cell->m_links` (swap and `pop_back`, `:230-235`) while `Finish`
   iterates the same vector (`:379`). That is a vector race that can read freed memory or skip a
   link, and it would also leave `m_linkIndex` of another adapter wrong.

For a template module the same drop is safe, because the module's context holds the adapter until the
destructor, which freezes the cells first (`template_module.cpp:327-334`). It is the render, and a
module call whose cell is released at an earlier point, that has the window.

TSan reproduces the first: `0338-memory-order/probe_unlink.cpp` (project files) renders
`{% set _ = l.append(1) %}{% set _ = keep([l, [l]]) %}` plus 30 more appends 20000 times while a
second thread drops what `keep` queued. One report per run in 3 of 3 runs: the read in `Unlink` (from
`~OwnedAdapter`, on the dropper) against the write in `Finish`. See
`docs/design/borrowed-cells-memory-order.md`, sections 8 and 9.

**Proposal.** Restore the invariant "the last reference to an adapter attached to a live cell is
dropped on the cell's thread" by construction, or stop the host from holding such an adapter.

- (a) Pin on attach. `Link` takes a strong reference to the adapter object and the registry releases
  it in `Finish`, so a host reference is never the last one while the cell is live. Cost: a
  reference count increment and decrement per attach, and `m_links` has to find the owner object
  (a virtual on the adapter or an `IntrusivePtr` alongside the pointer).
- (b) Do not export an attached adapter. The export of a template container that holds an adapter
  (`keep([l])`, an argument to a callback) stores a detached copy of it. This is also the first
  half of 0332's fix, and it closes the stale-value case as well; it costs a copy per exported
  adapter.
- (c) Make `Unlink` safe from any thread: `m_frozen` atomic (release in `Finish`, acquire in
  `Unlink`) and, for a live cell, a foreign thread defers the unlink to a per-cell lock-free list
  that `Finish` drains. The most code, and it adds an atomic to the path.

(a) is local and keeps 0332's choice open; (b) belongs with 0332 and should be decided there. A
decision is needed from Ruslan's side only because (b) changes what the host sees.

**Done when.**
- A TSan row from probe B (kept container dropped on another thread while the render runs, at least
  2000 renders) is clean, and fails on master.
- The ASan row of the same test is clean.
- The TLA+ model has a transition for a host thread dropping a value that holds an attached
  adapter, and `tla/check.py` fails without the fix (a mutant).
- `Module/keep` and the for-loop rows do not move in `bench/count.py`.
