---
status: open
priority: low
area: robustness
depends: [0508]
touches: [src/statements.cpp, src/render_context.h]
shares: [docs/security.md]
---
# The render memory budget leaves out a macro's function storage and a module context's vectors

**Problem.** After 0508 a render's scope maps, imported modules and closure captures count against
`maxRenderMemory`, but two parts of them still do not (massif on 5,000 with-context imports and 5,000 kept
closures, Debug, 2026-11-17): the heap block of the `std::function` a macro's `Callable` holds
(`MacroStatement::CallableBody`, about 124 bytes a macro, counted only by the `ValueBox`'s own size), and the
vectors inside an `ImportedModule`'s `RenderContext` (scopes, views, captures), below 1% each in the profile. Under
`UntrustedSettings`, 100,000 with-context imports peak at 324 MB with 256 MiB counted: the rest is these and the
process baseline.

**Proposal.** Count the callable body through `CountedAllocator` (a small owned functor instead of
`std::function`'s heap, or `allocate_shared` for its state), and give `RenderContext`'s vectors the same
allocator where a module or a closure keeps them. Measure with bench/count.py; Render/macros and the chat rows
are sensitive.

**Done when.** A loop of 50,000 kept macros fails at a budget within 10% of its measured peak over baseline, and
the bench rows stay within the perf bar.
