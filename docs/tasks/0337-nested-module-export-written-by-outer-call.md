---
status: open
priority: medium
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, test/borrowed_cells_test.cpp]
---
# A nested module call's kept value is written by the outer call

**Problem.** Decision D8 of 0331 promises that what a template module's call hands to the host is
a snapshot that no call writes once the call that handed it out has returned. A cell records one
call, the first one that exported its items (`BorrowedCell::m_exportedIn`). `NoteExport` keeps an
older mark, and `Unshare` writes in place when the mark equals the running call. A call nested in
another call of the same module (via a host callback) breaks the promise:

1. Call 1 (outer) runs `l.append(1)` and then `keep(l)`. The mark becomes 1.
2. Call 1 calls a host callable, which calls the module again. Call 2 (nested) runs `keep(l)`
   and does not write. `NoteExport` sees a mark and leaves it at 1, so call 2's kept value
   shares call 1's items.
3. Call 2 returns, and `EndCall` restores call 1's id.
4. Call 1 runs `l.append(2)`. The mark equals the running call, so the write goes in place.

Call 2's kept value now reads `[0, 1, 2]`, although call 2 ended at `[0, 1]`. A host thread
reading it races with call 1, which is shared mutable state across the host boundary (Ruslan's
rule, 2026-10-26). No allocation failure is needed.

Found by the TLA+ model of the cell protocol (`tla/`, finding F1, run `f1-nested-export`).
Pinned by `BorrowedCellsKept.ModuleNestedKeepOfTheOuterCallsExport`, which asserts the current
behaviour and gives the target in its comment.

0335 is the same root (one mark per cell) seen from sequential calls. There it only makes a
kept value miss its own call's later writes, and nothing is written after its call ends. This
task covers the nested case, where an ended call's snapshot is written.

**Proposal.** When `NoteExport` meets a mark from another call (`m_exportedIn != 0 &&
m_exportedIn != m_registry->m_call`), it sets a sentinel mark that no call id equals
(`UINT32_MAX`; `BeginCall` already skips 0, and must skip the sentinel too). Every later write
then goes through `Unshare`'s uniqueness test: it copies while a kept value still shares the
items, and clears the mark when none does. The cost falls only on a re-export from a different
call; `Module/keep` and `Module/keep_written` export once per call and never reach it.

The model checks the proposal as `FixNestedExport`. The pass scopes of `tla/check.py` run with
it on, and `f1-nested-export` runs with it off.

Call 1's own kept value still misses call 1's writes after the nested call. That is the 0335
limit, and 0335's per-call holder proposal must keep this case copying: a holder of another
call that shares the items forces a copy even when the mark is the running call's.

**Done when.**
- `ModuleNestedKeepOfTheOuterCallsExport` expects `[0, 1]` for call 2's kept value.
- A dict row does the same.
- The ASan and TSan rows are clean (two threads: the host reads call 2's value while call 1
  writes).
- `tla/check.py --only f1-nested-export` still finds the counterexample, and the pass scopes
  stay clean with `FixNestedExport` matching the code.
- `Module/keep` and `Module/keep_written` do not move in `bench/count.py`.
