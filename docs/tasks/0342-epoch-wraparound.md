---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/713
priority: low
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/internal_value.cpp, src/cell_trace.h, src/cell_trace.cpp, test/borrowed_cells_test.cpp, tla/check.py, tla/README.md, tla/CellProtocol.tla, docs/design/borrowed-cells-memory-order.md]
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

**Resolution.** A fourth option, cheaper than the three above: widen the epoch to 64 bits and keep
`CellLink` at two words. The stored epoch and the link's position fields were never used at the
same time: an attached link always takes `Find` (its epoch was the `Attached` mark) and a detached
one has no position in a cell's list (its index was 0). So `CellLink::m_state` is one 64-bit word
holding either the seen epoch or `AttachedBit` (bit 63) with `OwnsBit`, `HeldBit` and the index.
The counter (`fetch_add` now, no CAS loop), `ItemAdapters::quietEpoch` and `CellStep::epoch` are 64
bits. The counter would need 2^63 moves to reach the attached mark (292 years at 10^9 a second), so
it never returns to a stored value: the unbounded counter the TLA+ model assumes.

Measured cost (`scripts/preflight.sh --perf` against the 0339 commit, `bench/count.py`): every
benchmark between -0.05% (`Render/chat_llama`) and +0.00%, including `Render/for_loop_vars`,
`Module/keep` and `Module/keep_written`; allocations unchanged. Adapter sizes are unchanged on 64-bit
targets (the link stays 16 bytes; on a 32-bit target it grows from 12 to 16).

Evidence: `BorrowedCellsSingle.AdapterFindsACellPromotedAcrossThe32BitWrap` moves the counter by
2^32 - 1 through the exported `detail::AdvanceCellEpochForTesting`; on the 32-bit code it fails with
`[1]|[1]|[1, 9]` (the adapter skipped the lookup), with the fix it passes (skipped in the trace build,
whose epochs must fit TLC's 32-bit integers). The model's `open-0342-epoch-wrap` run is now the mutant
`epoch-wrap` (`MaxEpoch = 2`, counterexample `AliasesAgree`), and the design document's E4 and gap 3
are closed.
