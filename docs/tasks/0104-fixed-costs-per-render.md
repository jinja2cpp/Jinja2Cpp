---
status: done
priority: medium
area: perf
depends: [0011]
touches: [src/global_functions.cpp#SetupGlobals, src/template_impl.h#Render, src/render_context.h, src/internal_value.h#Callable, src/expression_evaluator.cpp#CallExpression, src/statements.cpp#MacroStatement]
---
# Fixed allocations per render and per macro call

**Problem.** Found with the allocation counts of the benchmark suite (0011). Rendering
`plain_text`, a template with no tags at all, takes 12 allocations and about 6,800
instructions; `substitute` (one `{{ }}`) the same 12. The heap profile shows where:

- `SetupGlobals` builds `range`, `dict`, `cycler`, `joiner`, `namespace` and `lipsum`
  as new `Callable`s (each a `std::function` behind a `RecursiveWrapper`) on every
  render, plus a `std::minstd_rand` for `lipsum`: 7 of the 12 allocations.
- `TemplateImpl::Render` and `TemplateRenderer::Render` allocate the rest (the global
  scope map, the parameter adapter).

Macro calls pay a similar fixed price: `Render/macros` (200 calls of a four-parameter
macro) makes 2,729 allocations per render, about 13 per call, in
`MacroStatement::InvokeMacroRenderer` and `CallExpression::Render` (argument vectors,
the callable wrapper, the macro's scope map, string growth of its captured output).

**Ideas.** The built-in globals are immutable and do not depend on the render: build
them once (a static table or one per environment) and look them up as a last scope
instead of copying them in. `lipsum`'s generator can live in the render context. For
macros, reserve the argument vectors from the macro's parameter count and reuse the
scope between calls the way 0087 reused the loop body scope.

**Done when.** Each idea is landed or rejected with a `bench/count.py --baseline`
measurement; renders of trivial templates need at most a handful of allocations.

**Outcome.** Landed in the PR for this task, measured with `bench/count.py --baseline`
against master 9548fa2 (instructions per render, allocations per render):

| Change | plain_text | substitute | macros |
|---|---|---|---|
| master | 6,794 / 12 | 8,814 / 12 | 2,564,733 / 2,729 |
| builtins in one static table, looked up after the global scope | 2,633 / 4 | 4,651 / 4 | 2,560,116 / 2,721 |
| `Callable` shares its function between copies | | | 2,408,614 / 2,322 |
| macro arguments bound from the call in place | | | 1,780,361 / 922 |
| a callee named by a variable is not copied into a new value | | | 1,622,081 / 522 |
| argument vector reserved | | | 1,582,481 / 422 |

- The built-ins (and the i18n functions) are built once in `GetBuiltinGlobals` and
  searched after the environment's globals through a new last scope of `RenderContext`,
  which `FindValueSlot` never returns, so the shared table is never written. lipsum's
  generator lives in the render's `RendererCallback`: one sequence per render, included
  templates share it, and every render draws the same text as before.
- `Callable` keeps its `std::function` in a `shared_ptr<const ...>`: copying a macro used
  to copy its captured defaults; creating a callable costs one allocation more (the
  control block), which `Render/inheritance` (`super`, `self`) still nets at -50.
- `InvokeMacroRenderer` no longer copies the keyword map or builds `args`/`isProvided`
  vectors; when no default refers to the other arguments, a missing argument with a
  default is not first bound to a hinted undefined.
- Every render benchmark gets cheaper or stays flat; the biggest are `macros` -38% and
  `strings` -2%.

Not done, measured: the 4 allocations left in a trivial render are the scope deque of
`RenderContext` (2), the output string and the parameter conversion; a `std::deque`
replacement must keep references stable across `EnterScope`, which the macro code relies
on. Reusing the macro scope between calls (the 0087 idea) would save at most the one
robin_hood node allocation per call left on `macros` (about 200 per render); it needs a
free list of scopes in the render context and was left out of this change.
