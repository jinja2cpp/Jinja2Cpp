---
status: open
priority: low
area: robustness
depends: [0256]
touches: [src/value_methods.cpp, test/allocation_failure_test.cpp]
---
# A failed list.sort() loses the list's items when memory stays spent

**Problem.** Found by the exhausted mode of `jinja2cpp_alloc_tests` (0256, the
`jinja2cpp_alloc_tests_exhausted` run). `ListSort` (src/value_methods.cpp) takes the list's
items, calls the key function, and its `Restore` guard puts the items back when a key call or the
sort throws. When a key call made a module call that handed the (then empty) list to the host,
`MutableItems(self)` in the guard copies the list on write (docs/tasks/0331, D8) so the host's
value is never written again. That copy allocates; with memory spent it throws, the guard's
`catch (...)` swallows it, and the list is left empty instead of "as it was".
`AllocationFailureTest.ModuleSortKeepsItsItemsWhenTheCopyFails` shows `[]` at allocation 30 in
the exhausted mode (the N-th-only mode passes: the copy succeeds there).

The render still ends with `std::bad_alloc` and nothing leaks; only the module's list is
wrong for later calls.

**Proposal.** Make the restore allocation-free: get the copy-on-write storage the guard may need
before the key calls (a spare storage, or `MutableItems` before taking the items when the list is
shared), so that the guard only swaps. Keep D8: the items the host was handed are never written
again. Check `bench/count.py` for `sort` with a key; the spare is made only for a shared list.

**Done when.** The allowance marked "task 0485" in
`AllocationFailureTest.ModuleSortKeepsItsItemsWhenTheCopyFails` is removed and both
`jinja2cpp_alloc_tests` runs pass.
