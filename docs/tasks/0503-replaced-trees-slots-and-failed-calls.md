---
status: open
priority: low
area: robustness
depends: [0482]
touches: [src/template_impl.h, src/template_slots.h, src/template_module.cpp]
shares: [test/allocation_failure_test.cpp]
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
