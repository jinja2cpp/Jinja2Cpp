---
status: open
pr:
priority: low
area: robustness
depends: [0529]
touches: [src/statements.cpp, test/allocation_failure_test.cpp]
---
# A `self` inside a list or dict at an include's top level keeps every replaced tree

**Problem.** Found by the verifier of 0529. 0529 weakens a `self` a name of an included template's top level holds
(`{% set q = self %}`) when the include's names are copied for its kept closures, so the copy no longer keeps the
capture that keeps it. A `self` held inside a container (`{% set l = [self] %}`, `{% set d = {'s': self} %}`) is
copied as the container, whose `self` still keeps the capture: the same cycle. With `TemplateLookup::EveryUse`, no
cache and a source changing before every call, a `Template::MakeModule` macro that includes such a template keeps
every replaced tree: 3,000 calls peak at 31.3 MB RSS (Debug) against 13.5 MB for `{% set q = self %}`. Master
behaves the same: memory only, no safety issue, output matches Jinja2.

**Proposal.** Let the module's call collector see the copy (a cycle through the capture: copy -> container ->
`self` -> capture -> copy is collectable once nothing outside holds the capture), or weaken a `self` found inside
the containers the copy holds, with a bound on the walk.

**Done when** `ModuleCallTest.EveryUseStoredSelfLetGoOfStaysFlat` has the list and dict rows and they stay flat, clean
under address+undefined.
