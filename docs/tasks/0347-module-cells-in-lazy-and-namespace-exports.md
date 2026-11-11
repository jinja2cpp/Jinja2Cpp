---
status: done
priority: medium
area: robustness
depends: [0332, 0335]
touches: [src/borrowed_cells.cpp, src/internal_value.cpp, test/borrowed_cells_test.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/836
---
# Module cells reached through a lazy list or a namespace are not pinned

**Problem.** Since 0335/0332 a module call's export pins the cells it reaches when the call ends
(`CallExports::Pin`), and the kept value reads them by identity (`CallExportItem`). That covers a
borrowed list or dict, a template list or dict, and their nesting. It does not cover a value whose
items are produced on demand: the result of `dictsort`, `map`, `select` and the other lazy filters,
or a `namespace`. Their items reach a module cell's adapter only when the host reads them, so
`CallExportItem` sees no accessor it knows, falls back to `IntValue2Value`, and the adapter's
`CreateGenericList` runs `BorrowedCell::ExportList` later, on the host's thread, outside the call.
That path reads the cell's live storage. So `keep({'k': l}|dictsort)` and `keep(namespace(x=l))`
show the module's later writes to `l` in the kept value (D8 says the call's final state), and a
host thread that reads the kept value while the module writes races with the write (TSan reports
it in `BorrowedCell::ExportList` under `CreateGenericList`).

Found by the verifier on the PR for 0335/0332/0340/0341: the base commit fails the same rows (18 of
25 container shapes; the PR leaves 4, all of this kind).

**Proposal.** Pin these shapes too: when a module call exports a value whose accessor is neither a
borrowed adapter nor a template container, either (a) materialise it at the export (a lazy list is
walked once, into a template list, and a namespace's members are exported as a map), paying a copy
only for these shapes; or (b) give `ExportList` a way to find the export context of the call that
was running when the lazy value was made (the call's `CallExports` held by the lazy value), so a
later read goes through `CallExports::Storage`. (a) is simpler and matches D8; (b) keeps laziness.

**Done when.** `keep({'k': l}|dictsort)`, `keep(l|map('string'))`, `keep(l|select)` and
`keep(namespace(x=l))` in a macro that writes `l` after the keep show the call's final state, the
cross-thread read row of `ModuleCellInsideAKeptListReadFromThreads` for these shapes is TSan-clean,
and `Module/keep` does not move in `bench/count.py`.

**Resolution (2026-11-11).** Option (a), where it was still missing. A generator (`select`,
`map`, `batch`, `reject`) was already walked into a template list when the host got it, on the
call's thread: its items are what it made then, and a list among them is the module's data,
pinned at the call's end, so those shapes held. Two shapes read the cell live after the call:
a dictsort or `items()` pair (`CallExportItem` exported the pair's key and value with
`IntValue2Value`, outside the call's exports) and a namespace (`NamespaceAdapter` lacked the
running-call export a dict has). Both now go through the call's exports. Rows:
`BorrowedCellsKept.ModuleCellInsideALazyValue` (12 shapes, two calls each), `ModuleTupleToAKeepUndefinedCallable` (a pair or tuple stays a 2-tuple for a keepUndefined callable: `CallListView` now forwards `IsTuple`, which a module call's tuple lacked before, verifier round 1) and the
`ModuleCellInsideAKeptListReadFromThreads` TSan row for `[l]`, a dictsort pair, a namespace and
a generator's item. Documented in docs/api-2.0.md 3.10, item 9.
