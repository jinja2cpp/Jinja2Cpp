---
status: done
priority: low
area: robustness
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/695
depends: [0337]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, test/borrowed_cells_test.cpp]
---
# A module call id wraps and a stale export mark equals it

**Problem.** `BorrowedCells::BeginCall` hands out `++m_lastCall` as a 32-bit id and skips only 0
(`borrowed_cells.h:373`; 0337 adds the sentinel `UINT32_MAX` to the skip). A cell's `m_exportedIn`
keeps the id of the call that first shared its items with the host, and `Unshare` writes in place
when `m_exportedIn == m_call` ("the call that handed them out is the one writing"). A cell that was
exported in call k and not written by any later call keeps the mark. After 2^32 - 2 further calls
of the same module, call k + 2^32 - 2 has the same id: its first write goes in place into items a
kept value still shares. The kept value changes after its call ended and a host thread reading it
races with the write, which is shared mutable state across the host boundary.

It needs one module instance called about 4.3 billion times (a day at 50000 calls a second) and a
cell that no call wrote in that time. Rare, but a wrap is a hole in the guarantee that decision D8
makes without a condition, and the TLA+ model's unbounded call counter hides it.

**Proposal.** Make the id 64 bits: `m_call`, `m_lastCall` and `m_exportedIn` become `uint64_t`
(a cell is a 192-byte heap object, the compare on the write path stays one compare), which cannot
wrap. The alternative is to keep 32 bits and, when `m_lastCall` wraps, walk the registry and set
every nonzero `m_exportedIn` to the sentinel (the next write then goes through the
uniqueness test); that is a few lines but a path that runs once in 2^32 calls and so is never
tested in production.

**Done when.** A test starts the call counter near the limit (a test accessor on `BorrowedCells`)
and shows that a kept value from before the wrap is not written by a call after it; `Module/keep`
and `Module/keep_written` do not move in `bench/count.py`. The design document
(`docs/design/borrowed-cells-memory-order.md`, E5) and the TLA+ README drop the wrap-around from
their lists.

**Resolution (2026-10-29, PR #695).** Call-end pinning (0335) replaced the per-cell export mark: a
cell is shared by a flag (`m_shared`) that a call's pin sets, and the next write copies unless the
host let go. No code compares a stored call id with the running one any more; the id only names a
call in the trace and may wrap (`BorrowedCells::BeginCall`). The TLA+ model bounds it
(`MaxCallId`).
