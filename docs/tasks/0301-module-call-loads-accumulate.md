---
status: open
priority: medium
area: robustness
touches: [src/template_module.cpp, src/template_impl.h]
---
# A `Template::MakeModule` module keeps what each of its calls loads

**Problem.** A module made by `Template::MakeModule` (0284) runs every call of its macros on
one `RendererCallback`, which lives as long as the module. Whatever a call loads stays in that
callback's `LoadedTemplates` until the module is freed: the `ImportedModule` of an
`{% import ... with context %}` in a macro body (a new one per call) and the results kept in
`replaced`. A host that keeps one module and calls it per request grows without bound. The
verifier of #TBD measured `{% macro m() %}{% import 'lib' as l with context %}{{ l.x() }}{% include 'lib' %}{% endmacro %}`
at 11.9 MB RSS after 1k calls, 60 MB after 20k and 212 MB after 80k; a plain `{% import 'lib' as l %}`
(shared without context) stays flat at about 9 MB.

Freeing them at the end of each call is not safe as it stands: a macro of such an import
stored into a module namespace by the call would outlive its module's context, and the
ClosureSite rule only covers contexts that end with their render.

**Proposal.** Give each call a callback scope of its own whose modules end with the call,
with the 0038 weak-home rule for a macro that escapes into the module (a later call of it runs
with every scope hidden, or fails with TemplateExpired), or cap what a module keeps and fail
past it with ResourceLimitExceeded.

**Done when** the example above stays flat over 100k calls, with a test that a macro stored
across calls fails cleanly.
