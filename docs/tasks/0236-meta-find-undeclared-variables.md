---
status: open
priority: low
area: api
touches: [include/jinja2cpp/template.h, src/template_impl.h]
---
# Template introspection: `meta.find_undeclared_variables`, `Template.module`

**Problem.** `docs/parity.md` marks `jinja2.meta.find_undeclared_variables` and
`Template.module`/`make_module` as ❌ with no task. Hosts commonly use
`find_undeclared_variables` to learn which inputs a template needs (LLM chat-template
hosts check required variables this way). `GetMetadata` reads only the C++-specific
`{% meta %}` block. Raised by milestone 1.2's `meta` issue (#107), whose closure delivered
the `{% meta %}` block rather than introspection.

**Proposal.** A public API that returns the free names of a loaded template by walking
the parse tree (names read before any local binding, including in included templates
only when Jinja2's `meta` does); tests mirror `jinja2.meta`'s behaviour. Decide
`Template.module` (exported macros and top-level sets as a map) as do or won't do. An
additive API, so it can land in a 2.x minor (docs/release-2.0.md section 4).

**Done when.** The API exists with tests checked against `jinja2.meta`, and parity.md's two
rows name this task or a decision.
