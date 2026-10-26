---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/667
priority: medium
area: robustness
touches: [src/template_module.cpp, src/template_impl.h, test/template_module_test.cpp]
shares: [src/render_context.h, src/statements.cpp, src/error_info.cpp, include/jinja2cpp/error_info.h, test/allocation_failure_test.cpp]
---
# A `Template::MakeModule` module keeps what each of its calls loads

**Problem.** A module made by `Template::MakeModule` (0284) runs every call of its macros on
one `RendererCallback`, which lives as long as the module. Whatever a call loads stays in that
callback's `LoadedTemplates` until the module is freed: the `ImportedModule` of an
`{% import ... with context %}` in a macro body (a new one per call) and the results kept in
`replaced`. A host that keeps one module and calls it per request grows without bound. The
verifier of #634 measured `{% macro m() %}{% import 'lib' as l with context %}{{ l.x() }}{% include 'lib' %}{% endmacro %}`
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

**Plan** (approved by Ruslan 2026-10-25, option A + D2a + D3): /mnt/project-files/release-2.0/0301-plan.md.
Measured first: only the with-context import grew (279 MB after 100k calls); the include
and a plain import were flat.

**As built.** `TemplateModuleState::Call` takes a `CallScope`: it marks the callback's
`modules` and `invalidNames` (the error results of non-string names, split out of
`replaced`) and ends what the call added, newest first, except modules shared without
context, which stay for the module's life as Jinja2 keeps a template's default module
(D3), and the modules such a module's top level imports (`IRendererCallback::EnterSharedModule`;
found by the verifier: they had ended with the call while the shared module's macros still
ran in them). Trees stay: a stored macro views its tree. A module a call imports with context marks
its context (`RenderContext::SetEndsWithCall`); a closure defined there and called after
the call fails with `TemplateExpired`, extra params `[export name, "ended"]` (D2a), where
Jinja2 keeps the module alive and prints the macro's output. `HomeCall::Restore` puts the
module's context back as the call found it: a failed call had left its body's scopes there
(131 MB after 200k failing calls with a loop in the body, found while measuring).

Result: the task's example stays at 7 MB RSS over 400k calls; a failing call and an
invalid template name too. Follow-ups: 0312 (keep a module alive while a stored macro
reaches it, Python parity), 0313 (`EveryUse` reloads kept for a module's life).

