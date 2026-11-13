---
status: in-progress
priority: high
pr: TBD
area: lifetime
depends: [0324]
touches: [src/template_impl.h, src/cycle_collector.h, src/cycle_collector.cpp, src/template_module.cpp]
shares: [test/borrowed_cells_test.cpp]
---
# A macro a nested render writes into a shared global is called after its template is gone

**Problem.** Found by the security review of #870, while fixing 0463. Since 0324 a render that a host callable
starts during another render on the same thread shares the outer render's globals, and a global it
writes in place is the outer render's cell. A value it writes there can be a macro of the nested
template. If the host destroys that template before it returns, the outer render can call the
macro, whose `CallableBody` holds a raw `MacroStatement*`, an `ArenaView` of the freed tree and a
`ClosureSite` of the ended render:

```cpp
TemplateEnv env;
env.AddGlobal("g", ValuesList{});
Template outer(&env);
outer.Load("{% set _ = g.append(1) %}{% set _ = inner() %}{{ g[-1]() }}");
ValuesMap params;
params["inner"] = UserCallable([&env](const UserCallableParams&) {
    Template t(&env);
    t.Load("{% macro m() %}hi{% endmacro %}{% set _ = g.append(m) %}");
    t.RenderAsString({});
    return Value();
}, {});
outer.RenderAsString(params);
```

ASan reports a heap-use-after-free READ in `MacroStatement::CallableBody::operator()`
(statements.cpp, `macro->InvokeMacroRenderer`) on master 85e38c6. The fix for 0463 makes the outer render also see the write when the nested
render writes `g` first (drop the first `append`), so 0463 was taken out of #870 and lands with
this fix: its implementation, test and TLA+ change were built and checked there (commit 2a53069
on branch claude/project-thread-cj53va before the split; the diff is kept in the project files
as robustness-r1/0463-full.patch).
Memory corruption reachable from template text with a host that renders a template inside a
callable: P0 by REVIEW.md.

**Direction.** A callable (macro, `caller`, a closure) of a nested render must not reach a cell
another render reads after the nested render ended. Options: the nested render's values written to
an outer registry's cell keep their template and home alive (`SetKeepAlive` with the template's
owner and a captured site, as 0326 does for frames); or a callable whose home render ended raises
when called, as a host's copy of a macro does after its render; or a nested render's write of a
callable into a shared global raises. Check the same for `caller`, `loop` and `self` callables, and
for a nested render on the module path (0450).

**Done when.** 0463 is applied, and the repro and its nested-first variant render or raise under ASan with no report,
with a test in `test/borrowed_cells_test.cpp`; the chosen behaviour is checked against Python
Jinja2 where Python can express it.

**Resolution (Ruslan, 2026-11-13: keep the nested template alive).** A render that shares the
snapshot of the globals of a render it runs inside (`SharedSnapshotKeeper`: the outermost such
render on the thread) hands that render what its values need, so that a value it stores in a
global stays valid there until the outer render ends:
- its template (`TemplateImpl` is `enable_shared_from_this`) and each template it loads
  (`RendererCallback::KeepLoaded`), held by the outer render's collector (`CycleCollector::KeepTree`,
  `Notes::trees`, declared first so that it goes after the cells and the walk; a nested collector
  that has some hands them to its outer one, as it hands its notes). A tree is kept once
  (`Notes::treeSet`). When the nested render ends, what it kept goes again unless the keeper's
  registry holds a cell (`DropTreesUnlessWritten`): with none, no global was changed in place, so
  no value of the nested render can be where the outer render reads it, and a loop of nested
  renders of new templates stays flat (20000 renders of a 2 KB template: 43 to 47 MB);
- its params as owned copies (`OwnedArgument`, as a template module converts them) rather than
  views of the host's `ValuesMap`.
Before the fix ASan reported a use-after-free for 13 of 19 variants: a macro written outer-first
(the reproducer) and nested-first (0463), in a dict, a `caller`, a recursive `loop`, `self`, an
imported module, its macro and a `from` import, a macro defined in a macro, a param and an item of
it. A render that runs inside no other one pays one compare (`Running()`). Memory: once a
global was written in place, the outer render holds every distinct template a nested render loaded
or rendered until it ends, outside `maxRenderMemory` (the same loop: 47 to 122 MB); filed as 0492. Host data that a param only points to (a reflected pointer, a
`GenericList` accessor) stays the host's to keep alive, as for any value a render hands out.
`BorrowedCellsSingle.NestedRenderValueOutlivesItsTemplate`, `NestedRenderSelfOutsideItsRender`,
`NestedRenderValueOutlivesItsTemplateInsideAnotherEnvironment` and `WideNestedRenderMacroOutlivesItsTemplate` destroy the nested template and params before the
outer render reads the value; under ASan they report nothing. Python agrees but for closures of
the nested render's frames and its `self`, memory-safe, filed as 0491.
