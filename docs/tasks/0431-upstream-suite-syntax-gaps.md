---
status: in-progress
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

**Result.**
- Names: the lexer takes a name's characters from the XID_Start and XID_Continue tables of
  `src/unicode_tables.h` (generated from the pinned Python 3.12, Unicode 15.0), in narrow
  (UTF-8) and wide (UTF-16 or UTF-32) templates. A run Jinja2 rejects with "Invalid character in
  identifier" (`a²`, `·a`) is a parse error here too. Only ASCII goes through the locale's
  classes, so a global locale an embedder sets does not change what a name is. Cases
  `statements.name_*`. Python's compiler NFKC-normalises the names a template sets (`ﬁ` is `fi`);
  the engine does not: task 0446.
- Colon: `for`, `if`, `elif`, `else`, `block`, `macro`, `call`, `filter`, `autoescape` and a
  `set` block may end with `:`, as Jinja2's `parse_statements` allows (not `with`, an assigning
  `set`, `include` or an end tag). Cases `statements.colon_*`.
- `x if c` without `else` gives Jinja2's `cond_expr_undefined`: a default-policy undefined, whatever
  the environment's, with the hint "the inline if-expression on line N evaluated to false and no
  else section was defined." (Jinja2 adds ` in 'name'` for a named template; the engine's error
  location names it), made once at load. Cases `statements.if_expr_no_else_*`. A chained
  `a if b if c` is still a parse error: task 0447.
- `{% macro x(caller) %}` was already rejected; what the upstream test checks is the class
  (TemplateAssertionError), the message and a TypeError for two callers, which need an error
  code of their own: moved to 0432 with its expected-failures line.
