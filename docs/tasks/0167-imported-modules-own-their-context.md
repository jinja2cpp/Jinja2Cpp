---
status: done
priority: medium
area: parity
depends: [0165, 0155]
touches: [src/statements.cpp#import, src/render_context.h, src/render_workspace.h, src/render_workspace.cpp, src/template_impl.h]
shares: [src/statements.h, test/parity/cases/loader.py, test/import_test.cpp]
---
# Imported modules own their context; imported macros run in their module

**Problem.** An imported macro runs through `ImportedMacroRenderer`, which clones a context
per call and finds the module's names through `BindScope`/`m_boundDepth` and a
`$$_imported_*` lookup by name (0117 P0). `import ... with context` reads the call site's
names instead of the importer's at import time, and a macro imported into one module and
passed to another renders nothing. The wrapper costs 3.6% of `html_autoescape` and 6.0% of
`config_file` renders.

**Proposal.** Phase PR 3 of docs/design/0038-lexical-scoping-plan.md (section 4.5 as
amended by sections 10 point 5 and 11). The render owns each imported module as
`std::unique_ptr<ImportedModule>` in its per-render module list; the module renders into
its own heap `RenderContext`, whose external scope is a shallow snapshot of the importer's
visible names when imported with context. Exports are plain closures (0165) whose weak
anchor names the module context: they do not own it, so no cycle forms and no sweep is
needed. Delete `BindScope`, `m_boundScope`, `m_boundDepth`, `FindValueFull` and the
`$$_imported_*` lookup.

**Done when.** The loader rows of plan section 7 (PR 3) and the two snapshot rows of
section 10 point 6 (namespace mutated after import is seen, a re-`set` is not) pass;
`import_test.cpp` shows a module storing one of its own exports renders twice LSan-clean;
`ctest -R parity` passes; perf: html_autoescape and config_file improve, nothing else
outside ±0.5%.

**Next.** Modules imported in a loop live to the end of the render; if that ever matters,
free them per pass.
