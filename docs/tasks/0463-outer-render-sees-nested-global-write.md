---
status: open
priority: low
area: robustness
depends: [0324]
touches: [src/borrowed_cells.cpp, src/cycle_collector.h, src/cycle_collector.cpp, test/borrowed_cells_test.cpp]
---
# The outer render does not see a global a nested render changed first

**Problem.** Found while fixing 0324. A render started inside another one of the same environment
now shares the outer render's snapshot of the globals, so it sees a global the outer render changed
in place, and the outer render sees what the nested one writes to that global afterwards (both use
the outer render's cell). The 0211 decision says the outer render sees the inner one's changes in
general. It does not when the nested render is the first to write the global: the write promotes a
cell in the nested render's own registry (`CycleCollector::CellsForWrite`), which the nested render
detaches when it ends, so the outer render reads the environment's data again.

```c++
env.AddGlobal("g", jinja2::ValuesList{ 0 });
auto inner = env.FromString("{% do g.append(5) %}{{ g }}").value();
env.AddGlobal("nested", jinja2::MakeCallable([&] { return inner.RenderAsString({}).value(); }));
// renders "{{ g }}{{ nested() }}|{{ g }}"
```

Python prints `[0][0, 5]|[0, 5]`; Jinja2C++ prints `[0][0, 5]|[0]`. A loop over `g` in the outer
render that calls `nested()` in its body runs once here, where Python's sees the appended items.

**Proposal.** A nested render that shares the outer render's snapshot also promotes the cells of
the globals in the outer render's registry: the lookup and promotion of an adapter that is global
data (`IsGlobalData`) go to the registry of the outermost render that owns the snapshot, while
other data (params, which `BorrowedCellsSingle.NestedRenderHasItsOwnCells` keeps per render) stays
in the nested render's registry. Check it against the cell model (0338): the cell outlives the
nested render, and its links detach when the outer render ends.

**Done when.** The reproducer prints `[0][0, 5]|[0, 5]`, a render after it sees `g` as `[0]`, and
`NestedRenderHasItsOwnCells` still passes.
