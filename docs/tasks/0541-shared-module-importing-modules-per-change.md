---
status: in-progress
pr:
priority: low
area: robustness
depends: [0528]
touches: [src/template_impl.h, src/template_module.cpp, src/statements.cpp, src/statements.h, test/allocation_failure_test.cpp, test/template_module_test.cpp]
---
# A replaced shared module that imported modules at its top level stays for the module's life

**Problem.** Found while doing 0528. `RendererCallback::EndReplacedSharedModules` ends a module imported without
context once its template is replaced and nothing but the module itself holds what it made. It skips a module whose
top level imported modules (`Callback::Keeps`): those are kept for the importing module's life, and what it names of
theirs (`{% import 'helper' as h %}`, `{% from 'helper' import y %}`) makes its own values look held from outside to
`ImportedModule::HeldOnlyByItself`. So with `TemplateLookup::EveryUse`, no cache and a source that changes before
every call, a `Template::MakeModule` macro doing `{% import 'lib' as l %}`, where `lib` imports a helper library,
still keeps one module of `lib`, its tree and its helper modules per source change, as before 0528.

**Proposal.** Track which shared module owns each kept module (`KeptModule::owner`, set in `LeaveSharedModule`),
and check the group together: count the holders across the module and every module it owns (their names and module
objects), and each tree by its load, its module and its macros. End the group when it holds only itself; keep a
shared helper whose load is still the latest of its name, so a later import shares it as Jinja2 does.

**Done when** `ModuleCallTest.EveryUseChangingSourceStaysFlat` has a row whose `lib` imports a helper (with and
without context, `import` and `from`) and it stays flat, clean under address+undefined.

**Resolution.** The module's callback notes which shared module's top level imported which module, added or shared
(`Callback::imports`, from `LeaveSharedModule`; a shared hit is noted by `FindModule`). `EndReplacedSharedModules`
starts from a replaced shared module no other module imported and takes its group: it and what it imported,
transitively. A member whose load is still the latest of its name, or that a module outside the group imported, stays
shared with what it imported; the rest end together when `ImportedModule::HeldOnlyBy` finds every object of theirs
(names, the importer's names a module imported with context sees, the `import ... as` objects) held only among them,
or held by a staying member and not a macro of an ending one, and each of their trees held only by its load, the modules
made from it and their macros. Ending forgets the modules in `kept`, and a staying module that nothing imports any
more becomes a candidate of its own. Rows in `ModuleCallTest.EveryUseChangingSourceStaysFlat` (import and from,
with and without context, a helper that changes or stays, nested imports) and
`TemplateModuleChangingSourceTest.StoredMacroOfAnImportingModuleSeesItsHelper`.
