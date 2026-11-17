---
status: done
pr:
priority: low
area: robustness
depends: [0482]
touches: [src/template_impl.h, src/template_slots.h, src/template_module.cpp, src/statements.h]
shares: [test/allocation_failure_test.cpp, test/template_module_test.cpp]
---
# Replaced trees that hold a slot, or that a failed call loaded, still stay

**Problem.** Left by 0482. A module frees the trees a source change replaced at the end of each
successful outermost call, except (a) a tree that takes a slot (blocks, `extends`): `TemplateSlots`
links handles to it by its base address only (`TemplateSlots::Add`, `ArenaView::operator==`), so
it stays for the module's life, one per source change; and (b) the trees of calls that fail: the
pass runs only after a call that returned (`CallScope::Succeeded`), so a module whose calls keep
failing under `EveryUse` with a changing source keeps about 10.8 KB per failed call (verifier
measurement) until a call succeeds. A plain import also keeps one shared module per source change
(0284 decision D3).

**Proposal.** Give slots an owner or a generation tombstone (and make `Add` compare the owner or
size), then prune slot-holding trees as the others; for (b), run the pass on failure too once
the error a call returns is shown not to view a replaced tree (copy its location text first).

**Done when** `ModuleCallTest.EveryUseChangingSourceStaysFlat` gets rows for an `extends` child
and a block-using include whose source changes on every call, and one for a call that fails,
and they stay flat.

**Resolution.**
- (a) Only `SelfBlocks` (a `self` or `super()` that outlives its render) and an
  `ImportedModule`'s blocks or parent hold `TemplateHandle`s past a call.
  - In a module (a callback that keeps trees by value), `SelfBlocksOf` counts each `SelfBlocks`
    among the holders of its table (`TemplateSlots::Holders`, an atomic count shared with them).
  - `ImportedModule::HoldsTemplateHandles` tells a module that holds some.
  - At the end of an outermost call, when no holder of the table lives and neither the calling
    module nor a module its calls keep holds a handle, `PruneReplaced` clears the slots
    (`TemplateSlots::Clear`, a new generation on the next `Add`) and frees slot-holding trees like
    the others. Otherwise they stay, as before.
  - Remaining limits: a module whose own template has blocks or extends never clears its slots.
    A stored `self` or `super()` that is let go of stays counted, so the module keeps its slot
    trees from then on (0529).
  - Cost: a no-parent `super()` asks `TreeOwner` inside module calls, which scans the loaded
    names. It is free elsewhere.
  - The no-parent `super()` views its block name in the tree, and it keeps the tree's owner
    (`SetKeepAlive(TreeOwner())`, null outside a module).
- (b) A failed call prunes after its error is made and the exception is gone
  (`PruneAfterFailedCall`); the error is owned.
- The `ModuleCallTest` fixture added a file system per module, and lookups found the first one's
  unchanged files, so `EveryUseChangingSourceStaysFlat` reloaded only in its first row. It now
  shares one file system. The plain-import rows then grow, which is filed as 0528.
- Tests:
  - `ModuleCallTest.EveryUseChangingSourceStaysFlat` gets rows for an extends child, an included
    or imported template with blocks, and failing calls.
  - `TemplateModuleChangingSourceTest` gets rows (checked against Jinja2 3.1.6) for a module with
    its own blocks, a block inline in a module macro, a shared module's block, a stored `super`, a
    child of a changing parent, a stored `self` that is let go of, and a stored no-parent `super`.
