---
status: in-progress
priority: medium
area: parity
depends: [0049]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp#OutputValueConvertor, src/internal_value.cpp#CreateGenericList, src/internal_value.h#ValueObject, src/cycle_collector.h, test/borrowed_cells_test.cpp, bench/jinja2cpp_bench.cpp]
---
# What a host callable keeps of a container the render hands it

**Problem.** A user callable may keep a container the template passes it
(`{% set _ = keep(l) %}`). In Python the host then holds the render's own object, a mutable
object shared by the render and the host: it sees the render's later writes and keeps the
final state once the render ends. `l.append(3)`, `keep([l])`, `l.append(4)` leaves the host
with `[[1, 3, 4]]`.

We do not follow Python here (decision of 2026-10-26, a deliberate divergence). A shared
mutable object means data races or costly locks. A container the host keeps is never
mutable state shared between renders, threads or the host. The design question is which
immutable or host-owned value the host gets, when it gets it, and at what cost.

Since 0049 P2 the host's data is never written (D2): a render that writes to it works on a
per-render copy (a `BorrowedCell`), and the render's end detaches its adapters from the
cells. A container the render hands to a callable is a view at every call, as on master
(0049). While the render runs the host sees the render's writes through it; after the render
a host container the template wrote reads the host's original data, because the cell is gone.
On master the write had made the template's variable a copy, which the kept value shared and
which kept the writes, so this is a regression against master for that case
(`h.append(3)`, `keep([h])`, `h.append(4)`: master and Python keep `[[1, 2, 3, 4]]`, P2 reads
`[[1, 2]]` after the render). The host must also keep its params alive while it holds a kept
view. Master needed that only for a kept value the render never wrote; a written one held the
template's copy, so freeing the params after the render is now a use-after-free where master
was safe (a callable that renders an inner template with local params and returns the
argument it kept, `p.cpp` case 12 of the verifier).

**Target.** When the render ends, snapshot the containers that are both written (they have a
cell) and kept (handed to a callable while the render ran): the kept view then reads an
immutable copy of the render's final state that the host owns, instead of the host's
original, and outlives the params as master's copy did. A container that was only written, or only kept, costs nothing; a call costs no
copy. The snapshot is taken once, on the render's thread, before the cells are freed.

**Constraints, first one first.**
1. No shared mutable state: what the host keeps is never written by a render after the
   render ends, never written by two threads, and never reads a cell another render frees.
   A container a callable gives back to a later or concurrent render is that render's
   borrowed data like any other host data (copy on its first write).
2. No per-element cost on a render that never hands a container to the host, and no O(n)
   copy for a callable that only reads its argument during the call.
3. No growth of `CellLink` or of the adapters on the hot path. Every host-data row within
   +1% of master (`bench/count.py`, convert and reflect).
4. Cycles through a kept value, a cell and a host wrapper are freed or bounded, and leak no
   more than master (`l.append(idl(l))`, `keep(l)` after `l.append(l)`).
5. D2: the host's data is never written.

**Designs tried in #673, each rejected.**

1. Freeze at the render's end (852bc26, 079a6d2, 8a03956). The render kept export slots
   and, when it ended, walked what the host kept and copied it. The walk did not end when a
   host callable wrapped its argument in a new host list (`l.append(idl(l))` twice): each
   pass made a new host wrapper to walk. It also leaked more than master for cycles through
   host wrappers.
2. The host holds the render's own object (d1d06d9). A list or dict of the host's data
   handed out got its cell (`PromoteForExport`), the host held the cell's list or dict, and
   a list of the template handed out itself (`RenderOwnedAccessor`). This is Python's
   shared mutable object, and it gave two use-after-frees across threads. A container that a
   callable gives back from an earlier render is written by concurrent renders. And the
   promotion went through `IntValue2Value` (`generic_adapters.h`), which linked adapters of
   host data that several renders share to one thread's cell; another thread then read a
   cell its owner had freed. The cell copy it makes costs about 100 Ir per int element and
   270 Ir per dict item: +34% on a render of a 10k-int list that iterates it once
   (`bench/count.py`).
3. Copy at the call (fabf26e, removed from P2 again). While the render has cells, an export copies
   the render's view down to its nested containers (`CopyForExport`). It is safe across
   threads, but it copies on every call, also for a callable that only reads its argument:
   after one write to host data, calling a callable with a 100k-item host list costs about
   40 ms a call (master: microseconds). It also copies a list the template made, so a
   callable that returns its argument gives back a copy (`a.append(id(a))` appends a copy;
   master and Python append `a` itself). Rejected for the per-call cost.

**Open points for the design.** How the render records "kept" without growing `CellLink`
(a per-render list of the cells whose adapters were exported, filled only when a cell exists,
is one way); how the kept view finds its snapshot after the render (the export's adapter holds
the cell's link, so the snapshot can replace the cell's data instead of being detached); and
how the snapshot bounds cycles and depth, where design 1 failed (walk only the written cells,
never what the host made of the value).

**Done when.** An architect design picks one contract and records it, and the divergence
from Python, in `docs/api-2.0.md`; the implementation passes the five constraints with
TSan and ASan rows on four threads after the verifier's round 7 cases (`p7 16`, `p7 18`),
the leak shapes above, and `bench/count.py`.

**Plan.** Needs an architect design before any code: it changes what a value outside the
render owns, across threads. P4 of the 0049 plan (keys, callables, `id(l) is sameas l`)
works on the same boundary, so this design comes before P4 or together with it.

**Design.** Approved by Ruslan 2026-10-27: the architect plan
`/mnt/project-files/release-2.0/0331-plan.md` (option A, freeze in place with read-only exports),
the advisor's four changes, D1-D7 as recommended and D8 as copy on write.

**Implementation (PR #673, three commits).**
- A1, read-only exports: `CreateGenericList`/`CreateGenericMap` of the host-data adapters read
  the link (`CellLink::Current`) and never look a cell up. Attached, and not a global's
  adapter: the render's copy. Otherwise `ExportListView`/`ExportMapView`, a read-only view
  with no link and no item cache; a nested read is a new view with the same owner. A global's
  adapter (`ValueObject::IsGlobalData`: the `ByHeldRef` adapters, and the generic containers
  `LendHeldItem` makes) always exports the host's original, decided before its link is read.
- A2, freeze at the end: `BorrowedCells::Finish` drops each cell's source, freezes every cell,
  detaches a global's links (`HeldBit`) and turns every other link into a reference to its
  frozen cell. A frozen cell is never written (`MutableItems` null, `Adopt` refuses). A copy
  of an adapter that reads a frozen cell keeps it.
- A3 and D7: a link to a frozen cell is a reference the cycle collector follows (`OwnedAdapter`,
  the generic adapters and `ItemAdapters` report their frozen cells and item blocks, a frozen
  cell its storage), and `BorrowedCells::Release` hands the frozen cells that values still read
  to the collector ending the render or the module. A cycle nothing keeps is freed. A kept
  container that holds itself (`l.append(l); keep(l)`) is the render's copy, cycle included,
  and leaks once the host lets it go (D7 option a). Kept mutual cycles (`a.append(b);
  b.append(a); keep(a)`) stay cycles the same way: host conversion does not print them and
  they leak, where master printed `[1, [2]]` through the host's params and read freed memory
  once those were gone. Option b, which detached the
  back-edges at Finish, was dropped after verifier round 8: the inner name then read the
  host's params, a use-after-free once they were freed, and which edge broke depended on the
  table's order.
- The items of a copy keep the host's shared data alive through an adapter of their own
  (`LifetimeAnchor`), not through the adapter that reads the copy as its cell, which would
  make a cycle once the cell froze.
- D8: an export of a module cell notes its call; a write in a later call copies the items when
  an export still shares them (refcount test), so the host keeps an immutable snapshot of each
  call's final state. Bench `Module/keep`, `Module/keep_written`.

**Contract** (`docs/api-2.0.md`, values crossing the boundary, item 9). The host holds a read-only
snapshot, never shared mutable state:

| handed out | the host keeps after the render |
|---|---|
| after the render wrote it (`h.append(3); keep(h)`), or inside a container the template made | the render's copy, its final state; it outlives the params, except its items the render never wrote, which stay views of the host's data (as master) |
| before any write (`keep(l); l.append(4)`) | a view of the host's original, also during the render (Python: the writes; D1) |
| never written | a view of the host's original; the host keeps the params alive (as master) |
| a global's data | the global's original (D4; Python: the writes) |
| from a module call | that call's final state; a later call's write copies first (D8) |

Left: a container a module call made that holds a host container keeps reading the module's
live cell (task 0332).

A module call that hands out a container before its first write to it keeps the earlier call's
state (task 0335). The cells of host data that lives for one call (a host callable's result,
a generic container made per call) stay until the module goes (task 0336); a `ValuesList` or
`ValuesMap` argument of a module call is the call's own list or dict and makes no cell.
