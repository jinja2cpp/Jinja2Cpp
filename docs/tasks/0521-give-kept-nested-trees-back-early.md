---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/937
priority: medium
area: robustness
depends: [0492, 0481]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, src/template_impl.h, src/render_context.h, src/recursion_guard.h, src/recursion_guard.cpp]
shares: [src/statements.cpp, test/borrowed_cells_test.cpp, test/limits_test.cpp, bench/jinja2cpp_bench.cpp]
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

**Resolution (architect plan, steps 0 to 2; mechanism A).** The outer render no longer holds nested trees; the
values that name a tree own it, and the outer render's collector notes the tree weakly so that it still counts it.
- `RendererCallback::SetKeeper` turns on `KeepsTreesByValue`, so every macro, `caller`, recursive loop and no-parent
  `super` made in a nested render takes `RenderContext::TreeOwner()` (the host tree, `weak_from_this`, or one it
  loaded). The slot table `Templates()` gives under a keeper is a `SharedTemplateTable` that owns the trees its slots
  may name (each `KeepLoaded` tree, and the host's once a `self` or `super()` asks for the table); `SharedTemplates()`
  aliases its slots, so the `self` and `super()` stacks that outlive the render hold the trees too.
- Code of a nested tree that runs on another render's context (the outer render calls a macro of an ended nested
  render, a module calls a lent one) has no callback that knows the tree. `OwnedArenaSwitch(context, nodes, &owner)`
  sets a pair on the context (`m_ownerNodes`, a non-owning pointer to the owner the caller holds) that `TreeOwner()`
  prefers while `m_nodes` is that tree; the pointer targets the `CallableBody` or the loop-recursion lambda in the
  caller's copy of the callable, or a local of `RenderBlockAt` (the owner of an ended stack is its table). The pair
  is restored on exit, copied by the contexts a call copies and not by `CloneWith`; `SetNodes` leaves it alone, as
  a plain `ArenaSwitch` back to the tree must not lose it, and the view comparison makes a pair of another tree
  unused. A FULL contract check asserts the switches nest.
- `CycleCollector::Notes::trees` holds `weak_ptr` and the bytes charged (0492), with an address index. Notes of
  expired trees are given back (`ReleaseRenderMemoryOf` at the keeper's level, exactly what `ChargeRenderMemoryOf`
  returned in `taken`, clamped to the budget) when there are `max(64, 2 * live)` of them, when a tree does not fit
  (`RenderMemoryLeftOf`), and when `Keeps` finds one at the address of a new tree. `Finish` hands only live notes to
  the outer collector, never throwing past a tree it cannot note (the 0492 trap: the values own the tree, so a failed
  hand-over only leaves it uncounted).
- Measured: the 0492 loop (20000 nested renders of a new 2 KB template, no budget) keeps at most two live trees and
  fewer than 200 notes; it fits 1 MiB. A template that stores a macro of itself in a global still counts against the
  budget and fails the loop (`LimitsEnvTest`). `Nested/shared_globals` and `Nested/new_template` are the bench rows
  (`bench/count.py`, instructions per iteration, master then this change): `Nested/shared_globals` 156,267 then
  158,425 (+1.4%: the owner table, one owner copy per macro and the switch per macro call, in each of ten nested
  renders), `Nested/new_template` 449,277 then 447,114; `Render/plain_text` 959 then 962, `Module/keep` +0.3%.

Not done here: step 3 (single-pass `OwnedArgument`, `template_module.cpp`) and lazy params, filed as 0532. A stored
`self` that is let go of stays until the keeper ends (0529). Every new value that views a tree must take
`TreeOwner()`, and every node that switches trees `OwnedArenaSwitch` with its owner (REVIEW.md).

Verifier round 1: the ledger's insert reserved one more note per tree, which made a loop that stores a
macro per nested render quadratic (now pushed first, geometric); a row that pins `RenderBlockAt`'s owner
(a block that defines its macro only when the outer render calls it through `self`). The holders that
still outlive what names a tree (closures in `KeptObjects` until their prune, a dropped recursive
`loop`) are 0539. Also fixed: `LexScanner::EofToken` wrote its static token on each call, a data race
between threads loading templates (TSan).
