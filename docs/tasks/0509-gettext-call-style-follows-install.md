---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/936
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
Documented in the "Differences" list of docs/python.md. The C++ API shows the same mix the other way
(the 0415 verifier): flip `i18nNewstyle` on a warm environment, and a template included from the
cache keeps the `{% trans %}` tree of its first load while `gettext` resolves to the includer's
builtins, so `{% trans n=1 %}a {{ n }}{% endtrans %}` fails with `KeyError: 'n'`. Also: the fuzz
targets (fuzz/fuzz_common.h `ConfigureEnv`) set `i18n` but never turn `i18nNewstyle` off, so the
oldstyle parser path has no fuzz coverage; a settings byte choosing the style would add it.

**Proposal.** Let the call style be the environment's translations state, set by
`InstallGettextCallables` (for example an optional style argument, or the module installing
newstyle wrappers as globals of its own), and keep the load-time setting for `{% trans %}` blocks.
A public-API option needs a recorded decision; the module-only route does not.

**Done when.** `python/tests/test_i18n.py` has a row that loads under oldstyle, installs newstyle,
renders `gettext('%(a)s', a=1)` and matches Jinja2, and the docs/python.md line is removed.


**Resolution (batch 9 D).** The public-API route, additive: `TemplateEnv::InstallGettextCallables(GettextStyle,
...)` (docs/api-2.0.md). The module installs with the style of each `install_gettext_*` call, so the globals of
every template take it, as Jinja2's do; `{% trans %}` blocks keep their load style. The C++ overload without a
style keeps 0415's behaviour. Rows: `test_the_gettext_calls_take_the_style_of_the_last_install` (both orders,
calls and blocks) and `test_a_newstyle_install_formats_the_calls_of_a_template_loaded_under_oldstyle`;
`I18nOldstyleTest.InstallFixesTheStyleOfTheGlobals`. The docs/python.md line is gone. The C++ case of the 0415
verifier (a cached include keeps its oldstyle `{% trans %}` tree under newstyle globals) fails as Jinja2 does
with the same mix (`KeyError`, `incomplete format`), so it is not a divergence. A missing key is still
`TemplateRuntimeError` where Jinja2 raises `KeyError`: task 0511. Not done here: a fuzz settings byte for
`i18nNewstyle` (fuzz/ is outside this group's files; worth adding with the next fuzz task).
