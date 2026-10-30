---
status: open
priority: low
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp, test/borrowed_cells_test.cpp]
---
# The 32-bit cell epoch wraps and an old adapter compares equal

**Problem.** An adapter of the host's data stores the cell epoch it last saw (`CellLink::m_seenEpoch`)
and skips the lookup while it equals `CellCounters::Epoch()`. `ItemAdapters::quietEpoch` and
`CellStep::epoch` store one for a whole block or enumeration with the same rule. `NextEpoch` keeps
the counter off 0 and `UINT32_MAX`, so after exactly 2^32 - 2 moves it returns to every value it
has had. An adapter that stored `v`, was not read since, and finds the counter at `v` again after a
full cycle:

- misses a cell promoted for its data in the meantime and reads the host's data, so a render's own
  write is not seen through it (`Find` is skipped);
- or, for a quiet block, skips `Release` of its slots (a frozen cell is leaked) and `Traverse` (the
  collector does not see a cycle through it).

Each promotion and each render end that made a cell moves the epoch, so a server that renders
templates writing host data does about two moves per render. The adapters that live across that many
renders are the thread's globals snapshot and values the host keeps. The chance per read is about
2^-32, but a wrap is a hole in the invariant, and the TLA+ model's unbounded counter hides it.

**Proposal.** Decide with a measurement; none of these is free.

- Widen the stored epoch to 64 bits: `CellLink` grows from 16 to 24 bytes in every adapter and in
  every item adapter of a block. Needs a `bench/count.py` verdict on `for_loop_vars`, `Render/*` and
  the 0150 size goals.
- Keep 32 bits and make the counter per thread (`thread_local`): no cross-thread moves, so the wrap
  is 2^32 of this thread's own promotions, and quiet blocks recheck far less often under concurrent
  renders. It does not remove the wrap, and a thread-local costs a TLS access on the hot path of a
  shared library.
- Bound the age of an adapter instead: the globals snapshot is the only long-lived store, and
  `GetGlobalsSnapshot` could rebuild it when its recorded epoch is more than 2^31 behind. Values the
  host keeps stay unprotected.

**Done when.** The choice and its measured cost are recorded here. A test starts the counter near
the limit (a test accessor) and checks that an adapter read before the wrap finds a cell promoted
across it. The design document (E4) and the TLA+ README drop the wrap-around from their lists.
