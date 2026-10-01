---
status: open
priority: medium
area: parity
depends: [0001, 0018]
touches: [src/internal_value.h, src/internal_value.cpp, src/filters.cpp, src/statements.cpp, src/template_parser.cpp, include/jinja2cpp/template_env.h]
---
# Autoescape and Markup

**Problem.** Jinja2C++ has no autoescaping: no `autoescape` Environment option, no
`{% autoescape %}` block, no notion of a safe string, so `safe`, `forceescape` and the
`escaped` test cannot exist. Any HTML use relies on the template author writing `|e`
everywhere, which is the class of bug autoescape exists to prevent (26 cases).

**Proposal.** Add a "markup" flag to string values and Markup semantics (escaping the
other operand on `~`/`+`, `join`, `replace`, `format`; macros and block `set` return
markup; `safe`/`forceescape`/`e` respect the flag), a `Settings::autoescape` (bool, and
later a callback by template name like `select_autoescape`), and the block statement.
This touches the value model: start with an architect plan.

**Done when.** No line of `test/parity/divergences.txt` names task 0025, and `ctest -R parity` passes.
