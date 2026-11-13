---
status: open
priority: high
area: lifetime
depends: [0324]
touches: [src/statements.cpp, src/borrowed_cells.cpp, src/template_impl.h]
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
