---
status: in-progress
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/870
area: lifetime
touches: [src/internal_value.cpp]
shares: [src/borrowed_cells.h, src/borrowed_cells.cpp, test/allocation_failure_test.cpp]
---
# A failed module call's view of a module cell reads no items

**Problem.** A template module's call hands the host views of its cells (`CallListView`,
`CallMapView`, docs/tasks/0335). When the call ends, `PinExports` records the storages they
read. If that record runs out of memory, the call throws `std::bad_alloc` and records nothing
(0338 step D, failure-atomic), and the views read "data nothing writes": a nested item reads
the host's data through its adapter (`CallExportList`), but the view of a module cell at the
top has only the cell's identity, so it reads no items at all. `l` kept as `keep(l)` in a call
that failed at its pin shows `[]`, not the host's `l`. The value is stable and safe to read on
any thread; it is only not the host's data, which the comments of `CallExports` promise.
`FailedCallsPinIsAllOrNothing` in `test/allocation_failure_test.cpp` accepts `[]` for it.

**Proposal.** Give the top view the host's data to fall back to without keeping the module's
cell alive: for example the host accessor's `CreateHostGenericList()` taken at export time
(one allocation per export, measure Module/keep), or a reference to the host's data that does
not go through the adapter's link. Either way the fallback must never read the cell, which
the module thread keeps writing.

**Done when.** A failed pin leaves every view reading the host's data, the sweep case asserts
the host's value, and Module/keep stays within its gate.

**Resolution.** A view of a module cell (`CallListView`, `CallMapView` with an identity) gets the
host's data the cell copies when it is made: the adapter that found the cell passes itself to
`BorrowedCell::ExportList`/`ExportMap`, which takes its `CreateHostGenericList()`/`Map()` (one
allocation per export of a cell in a call). A view whose call recorded no storage for the cell
reads that, never the cell. `AllocationFailureTest.FailedCallsPinIsAllOrNothing` now accepts only
the call's final state or the host's `[[0]]`.
