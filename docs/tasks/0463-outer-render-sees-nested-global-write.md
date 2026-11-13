---
status: done
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/878
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

Python prints `[0][0, 5]|[0, 5]`; Jinja2C++ prints `[0][0, 5]|[0]`. The same holds when the outer
render only aliased the global before the call (`{% set y = g %}{{ nested() }}{% do y.append(1) %}{{ y }}`
with an empty `g` and a nested `g.append(4)`: Python `[4, 1]`, here `[1]`). A loop over `g` in the outer
render that calls `nested()` in its body runs once here, where Python's sees the appended items.

**Proposal.** A nested render that shares the outer render's snapshot also promotes the cells of
the globals in the outer render's registry: the lookup and promotion of an adapter that is global
data (`IsGlobalData`) go to the registry of the outermost render that owns the snapshot, while
other data (params, which `BorrowedCellsSingle.NestedRenderHasItsOwnCells` keeps per render) stays
in the nested render's registry. Check it against the cell model (0338): the cell outlives the
nested render, and its links detach when the outer render ends.

Related gaps seen by the 0324 verifier, same on master or new with 0324 but memory-safe:
- A host callable that calls `env.AddGlobal` mid-render moves the globals generation, so a render
  it then nests converts fresh globals and misses the outer render's in-place change (Python keeps
  one dict). Leave it, or match on the snapshot's source map instead of the generation.
- A macro that a nested render stores into the outer render's global, called by the outer render
  after the nested one ended, reports its closure variable undefined (Python reads it); compare
  0326 (closures of an ended frame).

**Done when.** The reproducer prints `[0][0, 5]|[0, 5]`, a render after it sees `g` as `[0]`, and
`NestedRenderHasItsOwnCells` still passes.

**Resolution.** A global's adapter (`IsGlobalData`) finds and promotes its cell in the registry of
the outermost render on the thread whose snapshot of the globals the running render shares
(`CycleCollector::GlobalCells`, `GlobalCellsForWrite`, through `RenderWorkspace::RunningGlobals`,
which now names each render's collector). Other data stays in the nested render's registry. The
cell outlives the nested render and its links detach when the outer render ends.
`BorrowedCellsSingle.OuterRenderSeesAGlobalANestedRenderWritesFirst` has the reproducer, the alias
and the loop, each with Python's output, and a render after each sees the environment's list.
The cell model has the rule (`FixGlobalOuterCell`, `AdReg`) and a new invariant,
`NestedGlobalKept`, which the mutant `no-global-outer-cell` breaks; `CellTrace.tla` accepts the
enclosing registry for a global's lookup and promotion. The two related gaps stay as written.
