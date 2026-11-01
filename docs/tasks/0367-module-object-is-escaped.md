---
status: open
priority: low
area: parity
depends: [0309]
touches: [src/testers.cpp#IsEscapedMode]
---
# `m is escaped` of an imported module

**Problem.** Found doing 0309. `{% import 'lib' as m %}` binds Jinja2's `TemplateModule`, whose
`__html__` gives its output as Markup. Since 0309 the module object prints that way under
autoescape (markup.h, `IMapAccessor::GetStrValue`), but the `escaped` test (testers.cpp,
`IsEscapedMode`) asks only Markup strings and host objects, so `{{ m is escaped }}` is False where
Jinja2 says True. Pinned by `loader.module_object_is_escaped`. testers.cpp belonged to parity
batch 2 group A when this was found, so it was not fixed in 0309's PR.

**Proposal.** In `IsEscapedMode`, also answer true for a map whose `GetStrValue()` is set.

**Done when** the line is gone from `test/parity/divergences/loader.txt`.
