---
status: done
priority: low
area: api
depends: [0236]
touches: [include/jinja2cpp/template.h, src/template_impl.h, src/statements.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/634
---
# `Template.module` / `make_module`: a template's exports as a map

**Problem.** Python Jinja2's `Template.module` (and `make_module(vars)`) renders the template
once and returns its exports: the top-level macros as callables and the top-level `set`
names as values, so a host can call `tpl.module.greet("x")` or read a constant the template
defines. `docs/parity.md` marks it ❌. Jinja2C++ builds such a module internally for
`{% import %}` (statements.cpp, `module.Render`), but the macros in it refer to the render
that made them, so handing them to the host outside a render needs the lifetime rule of
escaped macros (weak owner, `TemplateExpired`, docs/design/ from 0115/0137).

**Proposal.** `[[nodiscard]] Result<ValuesMap, CharT> MakeModule(const ValuesMap& params = {}) const`
on `BasicTemplate`, returning the exported names with macros as `UserCallable`s that keep the
template alive (or fail with `TemplateExpired`), plus the module's rendered body as Jinja2's
`str(module)` does. An additive API, so it can land in a 2.x minor.

**Done when** the API exists with tests checked against `Template.module` and
`make_module`, and parity.md's row names it.

**Done (decisions D1-D3 of the plan, /mnt/project-files/release-2.0/0284-plan.md).**
`Template::MakeModule(params)` returns `BasicTemplateModule{exports, body}`
(include/jinja2cpp/template.h). The module is an `ImportedModule` (statements.h) rendered on
a context of its own, with the template's params and a globals snapshot of its own, owned by
`TemplateModuleState` (src/template_module.cpp) which every exported callable keeps alive
together with the template (D1a). Calls take the state's recursive mutex and run under a
CycleCollector and the template's RenderLimitsScope; arguments are copied into owned values,
results copied out. A later `Load` makes calls fail with `ErrorCode::TemplateExpired` = 19
(D3), thrown as `BadResultAccess<CharT>` with the full error (D2a); a user callable that calls
an export inside a render passes that error on (CallHost). Names a top-level `import`/`from`
binds are not exported (`IRendererCallback::ImportBound`). Gaps: 0299 (re-set imports,
transitive exports of `import`), 0300 (callables passed in or nested, live values).
