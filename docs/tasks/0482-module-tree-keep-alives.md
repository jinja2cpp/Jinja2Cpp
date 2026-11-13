---
status: in-progress
priority: low
area: robustness
depends: [0313]
touches: [src/template_impl.h, src/statements.cpp, src/statements.h]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/884
shares: [src/template_module.cpp, src/template_slots.h, test/template_module_test.cpp, test/allocation_failure_test.cpp]
---
# End the trees a module replaced on a source change

**Problem.** With `TemplateLookup::EveryUse`, a `Template::MakeModule` module keeps one tree per source change of a
template it uses, for its whole life (0313 bounded only the unchanged reloads). The callback has to keep them,
because the values that view a tree hold raw views outside `TemplateSlots`: `CallableBody` (a statement pointer
and an `ArenaView`), the recursive loop's lambda and state, `SelfBlockCall` and `SelfBlocks` string_views,
`ImportedModule`'s frame name and handles. Freeing a tree any of them reaches is a use-after-free, and making them
fail with `TemplateExpired` diverges from Jinja2, which keeps the old `Template` alive: a stored macro of an
included template prints the old output on the next call and the new one after that (3.1.6).

**Proposal.** Keep-alives, as Python does: each value that views a tree holds the tree's owner (a module
callback's `TreeOwner(ArenaView)`, null in a plain render), and at the end of an outermost module call the
callback drops each replaced result that is not the latest for its name, not a shared module's key, not in
`bySite` and holds no slot. Full analysis, oracle rows and cost estimate: project file `0313/plan.md`. Needs a
recorded decision (cross-module, adds a member to macro bodies) and `bench/count.py`.

**Decision.** 2026-11-13, Ruslan: keep-alives as Python does (option c of the plan).

**Done when** a module called after N source changes of an included, imported (with context) or from-imported
template holds a bounded number of trees (`ModuleCallTest.EveryUseReloadsStayFlat` with a changing source), the
stored-macro and stored-`self` rows print what Jinja2 prints, and `TemplateModuleTest*`/`ModuleCallTest*` pass
under address+undefined.

**Next.** Trees that take a slot (blocks, `extends`) still grow per change until `TemplateSlots` gets owners or
generation tombstones (`Add` compares the base address only); a plain import keeps a shared module per change,
which only a change to 0284 decision D3 can end.
