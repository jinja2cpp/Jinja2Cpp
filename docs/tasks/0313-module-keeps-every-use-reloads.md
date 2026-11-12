---
status: in-progress
priority: low
area: robustness
touches: [src/template_impl.h]
shares: [src/template_module.cpp]
---
# A module keeps every tree `TemplateLookup::EveryUse` reloads

**Problem.** With `TemplateLookup::EveryUse`, each use of a template whose source changed
adds the new tree to the callback's `replaced` list, since a running render may still use
the old one. A `Template::MakeModule` module lives on across calls (0301), so it keeps one
tree per source change for its whole life. The growth follows source changes, not calls,
but a long-lived module over a template edited often still grows.

**Proposal.** End the old trees at the end of a module call once nothing views them: no
running call, and no stored macro whose `ClosureSite` names a context of that tree (0312
gives the reachability walk), or fail a stored macro of an old tree with `TemplateExpired`
as 0301 does for modules.

**Done when** a module called after N source changes holds a bounded number of trees.

**Resolution (scope narrowed).** Measured first: with `EveryUse` and `cacheSize = 0` a module kept a tree on every
call even when the source never changed (11 to 33 allocations a call), and an include in a loop kept one per
iteration, because a reload was compared by pointer. `RendererCallback::LoadTemplate` now drops a reload with the
same source, name and load settings (`TemplateImpl::SameParseAs`): it never ran, so nothing views it. Test:
`ModuleCallTest.EveryUseReloadsStayFlat`. Ending the trees of real source changes needs keep-alives in the values
that view a tree (macro bodies, recursive loops, imported modules), since those hold raw views outside
`TemplateSlots`; that cross-module part is [0479](0479-module-tree-keep-alives.md).
