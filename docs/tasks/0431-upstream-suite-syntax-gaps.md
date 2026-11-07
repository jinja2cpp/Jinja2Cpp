---
status: open
priority: medium
area: parity
depends: [0355]
touches: [src/lexer.cpp, src/lexer.h, src/template_parser.cpp, src/expression_parser.cpp]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt, python/tests/upstream/expected-failures.txt]
---
# Syntax gaps the upstream Jinja2 suite found

**Problem.** Running pallets/jinja 3.1.6 `tests/` through the Python module (0355) shows
templates Jinja2 parses, or rejects, that the engine handles differently. Each is pinned by its
line in `python/tests/upstream/expected-failures.txt`:

- Non-ASCII identifiers: Jinja2 accepts any name for which Python's `str.isidentifier()` holds
  (`{{ föö }}`, `{{ き }}`, `{{ a· }}`, `{{ ℘ }}`, `{{ ℮ }}`, `{{ ᢅ }}`); the lexer reports a
  parse error (`test_lexnparse.py::TestLexer::test_name[...]`, seven ids).
- A line statement may end with `:` (`% for item in seq:`), which Jinja2's lexer drops
  (`TestParser::test_line_syntax`, `test_line_syntax_priority`,
  `test_regression.py::TestBug::test_stacked_locals_scoping_bug`; the last also needs `print`,
  0419).
- `(1 if false).bar`: the else-less conditional gives an undefined whose attribute raises
  UndefinedError; the engine renders nothing (`TestSyntax::test_short_conditional_expression`).
- `{% macro x(caller) %}` without a default is a TemplateAssertionError ("\"caller\" argument must
  be omitted or be given a default"); the engine fails to parse the macro with another message,
  and `t.module.x(None, caller=...)` must raise TypeError
  (`test_regression.py::TestBug::test_double_caller_no_default`).

**Proposal.** Fix each in the lexer and parser, with a parity case in narrow and wide templates
(Unicode identifier table generated from the pinned Python, as `scripts/` does for the other
Unicode data), and delete the expected-failures lines.

**Done when.** The listed upstream tests pass through the module and the parity cases match.
