---
status: in-progress
pr:
priority: low
area: robustness
depends: [0503]
touches: [src/statements.cpp, src/statements.h, src/cycle_collector.h, src/internal_value.h, src/internal_value.cpp, src/template_module.cpp, test/allocation_failure_test.cpp, test/template_module_test.cpp]
---
# A `self` or `super()` stored in a module and let go of keeps the module's slot trees

**Problem.** Found by the verifier of 0503. 0503 clears a module's slots and frees its slot-holding trees only
while no `SelfBlocks` of its table lives (`TemplateSlots::HeldOutside`). With `TemplateLookup::EveryUse`, no cache
and a changing source, a call that stores `self` (`{% block b %}OLD{% endblock %}{% set ns.s = self %}` in an
included template) and a later call that sets `ns.s = none` should end that `self`. It does not: the `SelfBlocks`
stays counted until the module goes, apparently held in a cycle through its site capture
(`ClosureSite::CaptureOnEnd`, `RenderContext::KeepEndedSelf`). Every replaced slot tree is kept from then on. The
verifier measured 1,500 calls at 46.7 MB peak RSS, against 13.4 MB when `self` is never stored. A stored `super()`
with a parent (46.8 MB) and `{% set q = self %}` at an include's top level (89.8 MB) behave the same. Master
behaves the same, so this is not a regression or a safety issue. Output matches Jinja2.

**Proposal.** Find what keeps the `SelfBlocks` after the value that held it is gone: the capture the site keeps
while the `self` lives, which keeps the `self`. Break the cycle (a weak link from the capture), or let the
module's call collector see it.

**Done when** `ModuleCallTest.EveryUseChangingSourceStaysFlat` gets rows that store `self` (and `super()`) in
one call, let go of it in the next, and then change the source, and those rows stay flat. Clean under
address+undefined.

**Result.** Two causes. (1) A module keeps the copy of an ended frame's names for as long as the capture that owns
it lives (`KeptObjects`, `CycleCollector::KeepUntilEnd`), but released expired entries only when enough new ones
were kept (64): a `self` stored once and let go of left its copy, and the `SelfBlocks` in it, for the module's
life. `KeptObjects::ReleaseExpired` now runs when a module's outermost call ends (`CallScope`), before the slots
are asked who holds them. (2) A `self` a name of the included template's top level holds (`{% set q = self %}`)
went into that copy with blocks keeping the capture that keeps the copy: a cycle no call broke.
`MacroStatement::WeakenCapture` now weakens such a `self` (a `TemplateReference` of `SelfBlockCall`s, read through
`IMapAccessor::GetObjectItems`) as it does a macro, and `StrengthenCapture` makes its blocks keep the capture again
when a call reads it. `ModuleCallTest.EveryUseStoredSelfLetGoOfStaysFlat` has the stored `self`, `super()`, top-level
`self` and `set q = self` rows (26 to 34 allocations a call before); `TemplateModuleChangingSourceTest.SelfSetAtTheTopLevelOutlivesTheInclude`
pins the output (3.1.6). A weakened `self` read back is a new object: `q is q` across that read is not checked.
