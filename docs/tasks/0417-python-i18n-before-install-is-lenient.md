---
status: open
priority: low
area: parity
depends: [0354]
touches: [python/jinja2cpp/ext.py, python/src/environment.cpp]
shares: [python/tests/test_i18n.py, docs/python.md]
---
# `_()` and `{% trans %}` work before translations are installed

**Problem.** With `jinja2.ext.i18n` and no `install_*` call, Jinja2 has no `gettext` global:
`{{ _('x') }}` and `{% trans %}` raise `UndefinedError` (`'gettext' is undefined`), and so they
do after `uninstall_gettext_translations`. The engine always has null-translation globals
(task 0029), so the module renders the message untranslated.

**Proposal.** Decide whether this is a deliberate difference (a template that renders is
friendlier than an error nobody relies on) and record it on the divergence allow-list, or make
the module hide the engine's builtin globals until a call installs translations (a flag on the
engine's `TemplateEnv`, or Python globals that raise `UndefinedError`).

**Done when.** `test_without_installed_translations_the_engine_leaves_messages_as_they_are`
states the decided behaviour and docs/python.md says so.
