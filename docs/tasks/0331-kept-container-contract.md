---
status: open
priority: medium
area: parity
depends: [0049]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp#OutputValueConvertor, test/borrowed_cells_test.cpp]
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
cells. P2 ships the behaviour of fabf26e (see 0049): a container handed out before the
render wrote to any host data is a view, which reads the host's data again after the render;
one handed out after such a write is a deep copy made at the call. Both are defensible as
values; the copy's cost and its timing are not (below).

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

**Designs tried in #673, each blocked by the verifier.**

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
3. Copy at the call (fabf26e, what P2 ships). While the render has cells, an export copies
   the render's view down to its nested containers (`CopyForExport`). It is safe across
   threads, but it copies on every call, also for a callable that only reads its argument:
   after one write to host data, calling a callable with a 100k-item host list costs about
   40 ms a call (master: microseconds). It also copies a list the template made, so a
   callable that returns its argument gives back a copy (`a.append(id(a))` appends a copy;
   master and Python append `a` itself).

**Directions to evaluate.** Freeze on hand-out instead of at the end: the host gets the
render's container behind a reference-counted immutable snapshot, shared with the render
until the render's next write to it, which copies (copy on write, so a read-only callable
copies nothing and the render pays only when it writes after a hand-out). Or give the host a
view during the call and a snapshot only when the host keeps the value past the call, which
needs a hook on the host side that `Value` does not have today.

**Done when.** An architect design picks one contract and records it, and the divergence
from Python, in `docs/api-2.0.md`; the implementation passes the five constraints with
TSan and ASan rows on four threads after the verifier's round 7 cases (`p7 16`, `p7 18`),
the leak shapes above, and `bench/count.py`.

**Plan.** Needs an architect design before any code: it changes what a value outside the
render owns, across threads. P4 of the 0049 plan (keys, callables, `id(l) is sameas l`)
works on the same boundary, so this design comes before P4 or together with it.
