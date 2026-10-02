---
status: open
priority: low
area: api
depends: [0059]
touches: [include/jinja2cpp/template.h, src/template.cpp, README.md, test/]
---
# `[[nodiscard]]` on `Load` and `Render`, whose only output is an error

**Problem.** `modernize-use-nodiscard` (0059) marks only `const` member functions that
return a value, and 0059 added `TemplateEnv::LoadTemplate` and `FromString` by hand. What
remains unmarked are the functions whose only result is the error: `Template::Load` (both
overloads), `LoadFromFile`, `Render`, `RenderGeneric` (and the `TemplateW` versions), all
returning `Result<void>`. Dropping that result loses a parse or render error silently,
which is the failure the attribute exists to catch. But the README and many callers write
`tpl.Load(source);` without checking, so marking them warns (and fails `-Werror` builds)
in most existing user code.

**Proposal.** Decide (Ruslan) whether 2.0 marks them. If yes: add the attribute, change
the README and docs examples to check the result, fix the call sites in `test/`, and add
a line to the `[[nodiscard]]` release note in docs/api-2.0.md (section 5.5). If no: say
so there, so users know the omission is deliberate.

**Done when** the decision is recorded here and, if taken, the full CI matrix builds
warning-free with the attribute on.
