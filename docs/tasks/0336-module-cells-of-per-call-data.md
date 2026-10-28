---
status: open
priority: medium
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/cycle_collector.h#ModuleCallCollector, src/internal_value.cpp, test/borrowed_cells_test.cpp, bench/jinja2cpp_bench.cpp]
---
# A template module keeps the cells of host data that lived for one call

**Problem.** A template module's borrowed cells live across its calls (0049 P2), so that a
write to the module's host data in one call is seen by the next. A cell also keeps the
value promoted first (`BorrowedCell::m_source`), so that the identity the registry keys the
cell by is not reused while the cell exists. Both pull against data that lives for one call
only: a list a host callable returns in the call, or a `GenericList`/`GenericMap` the host
makes for each call's arguments. When the call writes it, its cell and the source stay in the
module's cells until the module goes: one more of each per call, with no bound.

`OwnedArgument` no longer makes such cells for `ValuesList`/`ValuesMap` arguments (#673, the
Codex review comment 4395529333): those become the call's own list or dict. What is left:
- a host callable's result written in the call (`{% set r = make() %}{% set _ = r.append(1) %}`),
  pinned by `BorrowedCellsSingle.ModuleKeepsTheCellsOfCallableResults` (current behaviour:
  two references per call; target: freed after each call);
- a generic container made for each call (`consume(MakeGenericList(...))`), whose accessor is
  the identity.

**Proposal.** At the end of a module call (`ModuleCallCollector`), drop a cell that nothing
but the registry holds, whose links are only its source's, and whose source alone keeps the
identity alive (its object and its shared data have no other owner). Dropping the source
detaches its link; then erase the cell. Data whose cells hold nested adapters of the same
root (a list of dicts) shares the root's ownership with those items, so the test must count
the references inside the group, or the cell keeps a weak reference to the data's owner
instead of the source and `Find` checks it (owner identity, so a reused address is never
taken for the old data). Generic containers need the accessor's owner, which `GenericList`
hides behind a `std::function` (public header: no ABI change without a decision).

**Done when.** The pinned test expects `use_count 1` after the calls, a row for a list of dicts
and one for a fresh generic list per call pass under ASan/LSan, a TSan row with two threads
calling the module is clean, and `Module/keep`, `Module/keep_written` do not move in
`bench/count.py`.
