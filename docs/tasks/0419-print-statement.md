---
status: in-progress
priority: low
area: parity
depends: [0354]
touches: [src/template_parser.cpp, src/template_parser.h, src/statements.cpp, src/statements.h]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt]
---
# `{% print expr %}` is a parse error in the engine

**Problem.** Jinja2 3.1.6 parses `{% print expr %}` (`Parser.parse_print`, a list of
comma-separated expressions output like `{{ }}`, with autoescape applied):
`Template('{% print 1, 2 %}').render()` gives `'12'`, and `{% print x %}` with autoescape
on and `x='<'` gives `'&lt;'`. Jinja2C++ reports an unknown statement. The
Python module (0354) handles it only by falling back to Jinja2 for the whole template, which the
fallback stats count as `loads_fallback`.

**Proposal.** Parse `print` as a statement that renders each expression as an output node
would (same escaping and finalize path), so the engine and the module render it natively.

**Done when.** A parity case for `{% print a, b %}` (with and without autoescape) renders as
Jinja2 does in narrow and wide templates, and the module renders it without a fallback.

**Result.** `print` is parsed where a statement name is expected (it stays a variable name
elsewhere, as in Jinja2): each comma-separated expression becomes an output node made as
`{{ }}`'s is (finalize, autoescape, volatile autoescape, dropped or guarded after `extends`, its
unknown filters placed as an output tag's). Parity cases `statements.print_*` (17). The Python
module's tests had used `{% print %}` as the syntax the engine lacks; they now use a subscript of
a literal by a tuple of slices, the one such syntax left (task 0445).
