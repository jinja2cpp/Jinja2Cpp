---
status: open
pr:
priority: medium
area: perf
depends: []
touches: [src/render_context.h]
---
# A loop that sets no names swaps empty scope maps

**Problem.** Found by the 2026-11-16 perf scout (master 9541b58,
`/mnt/project-files/perf-track/2026-11-16-scout.md`). Each run of a `for` loop enters and leaves two
scopes, the loop scope and the body scope (`ForStatement::RenderLoopInSlots`, src/statements.cpp
around lines 1553, 1580, 1638 and 1656). `RenderContext::EnterScope(InternalValueMap&&)` and
`ExitScope(InternalValueMap&)` (src/render_context.h) always swap the kept map with the pushed scope.
Since #891 (0231) the map is unordered_dense's segmented map, whose `table::swap` swaps the value
vector, the bucket storage and six more members. That costs about 19 instructions a swap, against a
few for robin_hood. A loop whose body sets no names swaps two empty maps that never had storage,
four times per loop run.

On `Render/mitsuhiko_table` (one inner loop per row, 1,001 loop runs) callgrind attributes about
77k instructions a render to inlined `std::swap` lines in `RenderLoopInSlots`. Last week's master
(8a573d2) had none. #891's faster name lookups hid the cost: the row is -1.12% for the week.

**Proposal.** Swap only when the map has storage, as the `RenderContext` constructor already does
for the top scope (`detail::HasStorage`, src/hash_map.h). In `EnterScope(map)`, swap only when
`HasStorage(map)`. In `ExitScope(map)`, swap only when `HasStorage(scope)`. A map without storage
is empty and holds nothing to hand back, so skipping its swap is exact.

A prototype is in `/mnt/project-files/perf-track/0518-scope-swap-prototype.patch`. Measured with
bench/count.py (GCC 13.3 Release, pinned name-hash key) against 9541b58:

- `Render/mitsuhiko_table` -1.37% and `mitsuhiko_table_wide` -1.28%, the same on their RenderTo and
  RenderSink rows;
- `Render/config_file` -0.52%, `hf_deepseek_v31` -0.23%, `for_range` -0.19%;
- no row above +0.03%, and no change in allocations or bytes.

**Done when.** The count job shows those rows down, with no row above +0.3% and no new allocations.
The loop tests pass under address/undefined sanitizers, in particular those for a body that sets
names in some runs and not in others (the kept map must still come back with its storage).

**Result.** `EnterScope(map)` swaps only when `map` has storage and `ExitScope(map)` only when
the scope has; a Debug contract check states that the kept map holds no storage when it is
handed back. Counted locally against 86815bd (GCC Release, pinned key): `mitsuhiko_table`
-1.37%, `mitsuhiko_table_wide` -1.28% (RenderTo/RenderSink the same), `config_file` -0.52%,
`hf_deepseek_v31` -0.23%, `for_range` -0.19%, no row up, allocations unchanged. Regression
case `ForLoopTest.NestedLoopSetsNamesInSomeRuns`.
