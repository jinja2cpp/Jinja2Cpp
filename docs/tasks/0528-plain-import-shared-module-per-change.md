---
status: done
pr:
priority: low
area: robustness
depends: [0503]
touches: [src/template_impl.h, src/statements.cpp, src/statements.h, src/template_module.cpp, test/allocation_failure_test.cpp, test/template_module_test.cpp]
---
# A module that plain-imports a changing template keeps a shared module per source change

**Problem.** Found while doing 0503. With `TemplateLookup::EveryUse`, no cache and a source that changes before
every call, a `Template::MakeModule` module whose macro does `{% import 'lib' as l %}` or
`{% from 'lib' import x %}` (without context) keeps about 19 to 22 allocations per call for its whole life. An
import without context shares one module per template (`ShareModule`, the callback's `sharedModules`), and a
shared module stays for the module's life, as Jinja2 keeps a `Template`'s default module. Each source change
makes a new template, so a new shared module and its tree, and the old ones are never let go of, while Jinja2's
old `Template` and its module go once nothing refers to them.

`ModuleCallTest.EveryUseChangingSourceStaysFlat` had both rows, but its fixture added a file system per row and
the environment asked the first one, whose files never changed, so they passed without reloading (fixed in
0503). They are left out of it now, pointing here.

**Proposal.** At the end of an outermost call (`PruneReplaced`), end a shared module whose template is no longer
the latest of its name and that nothing of the module's own reaches: not a kept module, not a value that holds
its owner (as 0482 does for trees), then drop its `replaced` entry with it.

**Done when** both rows are back in `ModuleCallTest.EveryUseChangingSourceStaysFlat` and stay flat, and the
stored-macro rows of `TemplateModuleChangingSourceTest` with a plain import print what Jinja2 3.1.6 prints, clean
under address+undefined.

**Result.** When the outermost call of a module returns and a source change replaced a tree,
`RendererCallback::EndReplacedSharedModules` ends each module shared without context whose load is no longer the
latest of its name nor a statement's (`bySite`), that holds no template handles, that is not kept by another
shared module's top level and did not import modules at its own (`Callback::IsKept`, `Keeps`), and that
`ImportedModule::HeldOnlyByItself`: every object of its names is held only by its names and, once made, the
`import ... as` object of its exports, and its tree only by the load, the module and the distinct bodies of the
macros among its names (a closure a call made there and stored, a stored macro or the stored module object keep
it, as in Jinja2). The cycles among its values are noted for the call's collector first; a failed call leaves
them to the next one that returns. `PruneReplaced` then frees the tree in the same pass. Both rows are back in
`ModuleCallTest.EveryUseChangingSourceStaysFlat`; `TemplateModuleChangingSourceTest.StoredMacroOfASharedModuleSeesItsNames`
has the stored-macro rows (3.1.6 `make_module()` results).
