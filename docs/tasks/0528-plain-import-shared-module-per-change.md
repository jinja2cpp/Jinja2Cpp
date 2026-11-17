---
status: open
pr:
priority: low
area: robustness
depends: [0503]
touches: [src/template_impl.h, src/statements.cpp, test/allocation_failure_test.cpp]
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
