---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/946
priority: low
area: robustness
depends: [0508]
touches: [src/internal_value.h, src/statements.cpp, test/limits_test.cpp]
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

**Done (2026-11-18).** `Callable` allocates its holder with `std::allocate_shared` and `CountedAllocator`, so the
holder and its control block count; a macro passes `sizeof(CallableBody)` and the holder charges it (when the render has
a budget) and gives back exactly that when the last copy goes, which covers the heap block `std::function` keeps the
body in. Measured with a counting `operator new` (malloc usable sizes) on the Debug build, 50,000 kept macros: peak heap
growth 19.3 MB, least budget that renders them 18.1 MB (94%); before, 17.7 MB of heap and 7.7 MB charged (43%).
`LimitsEnvTest.KeptMacrosCountAgainstTheBudgetClosely` fails at 17.5 MB (10% under the peak) and passes at 24 MB. The
vectors a module's `RenderContext` keeps were left uncounted: 10,000 with-context imports kept in a list peak at 51.4 MB
of heap and the least budget is 50.3 MB (98%), so they are about 2% and a counted allocator on the render path would
cost more than it gives (`KeptModuleListCountsAgainstTheBudgetClosely` records it); 20,000 kept closure copies: 34.6 MB
peak, 32.6 MB counted (94%). bench/count.py against the base: no row above +0.4% (Render/macros +0.01%, chat rows
-0.5% to +0.2%, plain_text and Module/keep unchanged), allocation counts unchanged.
