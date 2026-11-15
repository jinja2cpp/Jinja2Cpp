---
status: open
priority: low
area: parity
depends: [0415]
touches: [python/jinja2cpp/ext.py, python/src/environment.cpp, src/global_functions.cpp#SetupI18nGlobals]
shares: [docs/python.md, python/tests/test_i18n.py]
---
# The style of a gettext call follows the last install, not the template's load

**Problem.** Since 0415 the builtin `gettext`, `ngettext`, `pgettext` and `npgettext` globals take
the style of the settings a template was loaded with (`Settings::Extensions::i18nNewstyle`), as
`{% trans %}` blocks do. In Jinja2 a block takes the style at compile time, but the globals are what
the last `install_gettext_*` call put in `env.globals`: newstyle wrappers that format, or the raw
callables. So with the Python module, a template loaded while `newstyle_gettext` is off and rendered
after `install_gettext_translations(t, newstyle=True)` calls the translations without formatting:
`{{ gettext('%(a)s', a=1) }}` passes `a` to the callable (TypeError) where Jinja2 prints `1`. The
reverse order matches Jinja2, because an oldstyle install puts the callables in `env.globals`.
Documented in the "Differences" list of docs/python.md.

**Proposal.** Let the call style be the environment's translations state, set by
`InstallGettextCallables` (for example an optional style argument, or the module installing
newstyle wrappers as globals of its own), and keep the load-time setting for `{% trans %}` blocks.
A public-API option needs a recorded decision; the module-only route does not.

**Done when.** `python/tests/test_i18n.py` has a row that loads under oldstyle, installs newstyle,
renders `gettext('%(a)s', a=1)` and matches Jinja2, and the docs/python.md line is removed.
