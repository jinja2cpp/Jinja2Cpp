---
status: open
priority: medium
area: perf
depends: [0011]
touches: [src/global_functions.cpp#SetupGlobals, src/template_impl.h#Render, src/statements.cpp#MacroStatement]
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
