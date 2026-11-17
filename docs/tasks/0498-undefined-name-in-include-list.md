---
status: in-progress
priority: low
area: parity
depends: [0458]
touches: [src/statements.cpp#RenderByNames, python/src/errors.cpp]
shares: [python/tests/test_errors.py]
---
# An undefined name in an include's list loses its message in TemplatesNotFound

**Problem.** Found by 0458. `{% include [nope] %}` with `nope` undefined raises TemplatesNotFound in
both engines, but Jinja2's message names the undefined, `none of the templates given were found:
'nope' is undefined`, and `.templates` holds the Undefined object; through the module the message is
`none of the templates given were found: None` and `.templates` is `[None]`. The engine reports the
names tried as a public `jinja2::Value` list (ExtraParams[0]), which has no undefined value, so the
undefined arrives as None.

**Proposal.** Report the names tried so that an undefined one keeps its message: for example a third
ExtraParams entry with the message of each undefined name by its index, which the module turns into its
Undefined (with `_undefined_message`) before it builds TemplatesNotFound. Or record the difference as
deliberate (the class matches; only the text differs).

**Done when.** `{% include [nope] %}` raises Jinja2's message through the module, with a case in
python/tests/test_errors.py (`test_include_and_constructor_errors_have_jinja2s_class`).

**Resolution (batch 9 C).** An include that selects from a list adds ExtraParams[2] when names in it were
undefined: a list as long as the names, with the UndefinedError message of each undefined one and an empty value
for the others (documented on ErrorCode::TemplateNotFound). The module's `loaders._not_found` turns each into the
environment's undefined class with that hint, so TemplatesNotFound prints `none of the templates given were found:
'nope' is undefined` and `.templates` holds the Undefined (`IncludeTest.UndefinedNamesKeepTheirMessage`,
`test_include_and_constructor_errors_have_jinja2s_class`).
