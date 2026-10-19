---
status: open
priority: low
area: api
depends: [0236]
touches: [include/jinja2cpp/template.h, src/template_impl.h, src/statements.cpp]
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
