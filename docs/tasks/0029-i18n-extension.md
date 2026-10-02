---
status: done
priority: low
area: parity
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/324
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

**Result.** `Settings::Extensions::I18n` enables `{% trans %}` (parameters, `trimmed`/`notrimmed`,
a message context, `{% pluralize [name] %}`) and installs newstyle `_`, `gettext`, `ngettext`,
`pgettext` and `npgettext` globals with null translations; `TemplateEnv::InstallGettextCallables`
plays `install_gettext_callables(newstyle=True)`. A trans block compiles to the same gettext call
as in Jinja2 and looks the function up by name, so a `gettext` in the context replaces it. Corpus
area `i18n` (50 cases). Deliberate differences: the block's variables are evaluated once into a
scope of their own, so a call used as the count does not leak as `_trans`; a callable left out of
`InstallGettextCallables` keeps the message untranslated instead of leaving the global undefined;
the `ext.i18n.trimmed` policy does not exist (no policies API). Two cases wait on 0026
(StrictUndefined, calling an undefined `_`).
