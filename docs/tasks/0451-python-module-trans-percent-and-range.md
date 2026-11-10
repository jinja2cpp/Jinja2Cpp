---
status: open
priority: low
area: api
depends: [0354]
touches: [python/jinja2cpp/ext.py, python/tests/test_i18n.py, src/global_functions.cpp]
---
# Python module: `{% trans %}100%{% endtrans %}` prints `100%%`; `range()` prints nothing

**Problem.** Found by the security review of the 0354 code (2026-11-10, PR #823). Both are
parity gaps, not escapes:

- Through the module, with `install_null_translations(newstyle=False)`, the body of a `{% trans %}`
  block without variables keeps its escaped percent sign: `{% trans %}100%{% endtrans %}` renders
  `100%%`, where Jinja2 3.1.6 renders `100%`. The C++ corpus case `i18n.trans_percent_is_literal`
  passes, so the difference is in the module's oldstyle wrappers (`python/jinja2cpp/ext.py`,
  `_old_gettext` and `_format`), not in the engine.
- `{{ range() }}` renders an empty string in the engine and the module. Jinja2 raises
  `TypeError: range expected at least 1 argument, got 0`.

**Proposal.** Unescape `%%` in an oldstyle translation that has no variables, as Jinja2's
compiled call does. Make `range()` with no argument raise the TypeError Python does.

**Tests.** A row in `python/tests/test_i18n.py` comparing `{% trans %}100%{% endtrans %}` with
Jinja2 for both styles. A corpus case `globals.range_without_arguments` with the TypeError.

**Done when.** Both match Jinja2 3.1.6.
