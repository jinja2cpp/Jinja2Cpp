---
status: open
priority: low
area: parity
depends: []
touches: [src/statements.cpp, src/template_parser.h]
shares: [test/parity/divergences/filters.txt]
---
# Output after a top-level extends is not evaluated

**Problem.** Jinja2 compiles no output (`{{ }}`, template text, `trans`) of a template's body
after an `extends` at the top level (`has_known_extends` in jinja2/compiler.py), and after an
`extends` inside an `if` it guards each output with `if parent_template is None`. Jinja2C++
evaluates that output and discards it (TopLevelWriter in TemplateRenderer::RenderBody), so an
expression there that raises makes the render fail where Jinja2 renders the parent:
`{% extends 'a' %}{{ x|nonexistent }}` (`filters.unknown_filter_output_after_extends`; 0193
already skips its load-time check, as Jinja2 does), and any call with side effects runs.
Found by the 0193 verifier.

**Proposal.** Mark the output nodes of the body after a known extends at parse (the parser
knows it: TemplateRootInfo::knownExtends) and skip them, and skip the rest once `extends` has
run inside an `if`, instead of writing to a discarding stream.

**Done when.** No line of `test/parity/divergences/` names 0203.

**Also, load-time checks near `extends` (0193 verifier, rare).** A second top-level `extends`
stops Jinja2's compile of the body (CompilerExit), so unknown names after it outside blocks
are not reported (`{% extends B %}{% extends B %}{% macro m() %}{{ 1|f }}{% endmacro %}`
loads in Jinja2, fails in Jinja2C++; it fails at render in both). And Jinja2's i18n turns a
first `trans` variable that is a call into an assignment it compiles even after a known
extends (`{% extends B %}{% trans a=m(1|f) %}...` fails to load in Jinja2, loads here).
