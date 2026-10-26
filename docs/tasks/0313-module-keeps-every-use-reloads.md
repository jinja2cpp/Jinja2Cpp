---
status: open
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
