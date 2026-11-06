---
status: open
priority: low
area: parity
depends: [0029, 0354]
touches: [include/jinja2cpp/template_env.h, src/template_parser.cpp#ParseEndTrans, src/global_functions.cpp#SetupI18nGlobals]
shares: [python/jinja2cpp/ext.py, docs/python.md]
---
# Oldstyle gettext (Jinja2's default) in the engine

**Problem.** Jinja2's `newstyle_gettext` defaults to False: `gettext` is the translation function
itself, a call without variables returns the translation unformatted, and `{% trans %}` compiles
to `gettext(msg) % variables` (message ids with `%` unescaped when the block has no variables;
the result is Markup under autoescape). The engine implements only newstyle (task 0029): every
call formats with its keyword arguments, `{{ _('100%') }}` fails, and a trans block always
escapes `%` as `%%`. The Python module (0354) emulates oldstyle with Python wrappers installed
as the `gettext`, `ngettext`, `pgettext` and `npgettext` globals. They cannot tell a `{% trans %}`
block from a direct call, so they differ from Jinja2 where the two meet:
- `{% trans %}100% sure{% endtrans %}` (no variables) passes the message id `100%% sure` to the
  translations and prints `100%% sure`; Jinja2 passes and prints `100% sure`;
- `{% trans num=2 %}{{ num }} apple{% pluralize %}...{% endtrans %}` (the count is named `num`)
  prints `%(num)s pommes`: the engine passes no variable for it;
- under autoescape a `{% trans %}` block without variables prints its message escaped; Jinja2
  marks the translation safe;
- a direct `gettext('x %(a)s', a=1)` formats in the module and raises TypeError in Jinja2.

**Proposal.** A parse-time setting (`Settings::Extensions::i18nNewstyle`, default true for the
C++ API as today; the Python module sets it from `env.newstyle_gettext` when a template loads, as
Jinja2 reads it in `_make_node`). In oldstyle `ParseEndTrans` emits Jinja2's node (the unescaped
message when no variable is referenced; `MarkSafeIfAutoescape(gettext(...)) % {variables}`
otherwise) and the builtin globals call the installed callable without formatting. Public
`Settings` gains a field: needs an ABI note (abi/).

**Done when.** The four cases above match Jinja2 in `python/tests/test_i18n.py` (the `oldstyle`
rows), and the "Differences" line of docs/python.md is removed.
