---
status: open
priority: medium
area: robustness
depends: [0492, 0481]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, src/template_impl.h]
---
# Give the trees kept for nested renders back before the keeper ends, and copy their params lazily

**Problem.** Split from 0492. Since 0481 the outermost render that shares its globals with a render
nested in it keeps the nested template, the templates it loaded and owned copies of its params until
it ends (`CycleCollector::KeepTree`), whatever the nested render stored. 0492 counts each kept tree
against the keeper's `maxRenderMemory`, so a loop of host Loads now fails the render instead of
growing without bound, but a render with no budget, or a large one, still holds one tree per nested
render of a new template: 20000 nested renders of a 2 KB template took the process from 45 to
122 MB (0492). Two versions that gave trees back when nothing could have escaped each had a
use-after-free (0481 Resolution).

Also: a nested render that shares the snapshot copies its params (`OwnedArgument`) on every render,
O(size) per render, and converts them twice.

**Proposal.** Ownership rather than reachability: a value that names a tree (a macro's body, `caller`,
loop recursion, `self`, a view of its text) holds a keep-alive when its render has a keeper, or a
per-tree count of live callables is checked when the nested render ends, and a tree nothing names is
released then (crediting what 0492 charged, under 0483's rule). An architect plan first, then the perf
gate (`bench/count.py`, Module and nested rows). Copy params lazily, when a value of them is stored
where it can outlive the render.

**Done when.** The 0492 loop with no memory budget stays in constant memory when the nested render
stores nothing; the 0481 tests in `test/borrowed_cells_test.cpp` stay ASan clean; the bench rows stay
within the perf bar.
