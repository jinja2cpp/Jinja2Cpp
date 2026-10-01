---
status: open
priority: low
area: parity
depends: [0001, 0021]
touches: []
shares: [src/statements.cpp, src/template_parser.cpp, include/jinja2cpp/template_env.h]
---
# i18n extension

**Problem.** `jinja2.ext.i18n` adds `{% trans %}`/`{% pluralize %}` and the `_`,
`gettext`, `ngettext` globals. Jinja2C++ has none, so templates from translated
projects do not load (3 cases).

**Proposal.** Parse `trans` blocks into a message id with placeholders and call a
user-supplied translation callable (null translations by default, like
`install_null_translations`). Low priority until a user asks.

**Done when.** No line of `test/parity/divergences/` names task 0029, and `ctest -R parity` passes.
