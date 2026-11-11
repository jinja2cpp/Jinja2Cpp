---
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/845
status: in-progress
priority: low
area: parity
depends: []
touches: [src/expression_parser.cpp, src/lexer.h]
shares: [test/parity/cases/statements.py, test/parity/cases/whitespace.py, test/parity/divergences/statements.txt, test/parity/divergences/whitespace.txt]
---
# Chained else-less conditionals and Unicode spaces inside tags

**Problem.** Two load errors where Jinja2 3.1.6 parses, found by the 0431 verifier:

- `a if b if c`: `Parser.parse_condexpr` loops on `if`, so it is `(a if b) if c`
  (`{{ 1 if true if true }}` renders `1`). Jinja2C++ parses one `if` and reports the second
  (`statements.if_expr_chained_if`).
- A Unicode space between the tokens of a tag (`{{<U+00A0>x }}`, `{{ x<U+2003>}}`): Jinja2's
  `whitespace_re` is `\s+`, Unicode included. The engine's tag lexer skips ASCII whitespace
  only and reports a lexing error (`whitespace.tag_unicode_space`). Its tag splitter already
  knows Python's `\s` (`SpaceLengthAt`), for lstrip_blocks.

**Proposal.** Loop on `if` in `ParseFullExpression` as Python does, with the operator budget;
skip `unicode::IsSpace` code points in `TagLexer::SkipWhitespace` (narrow UTF-8 and wide), out of
line so the ASCII path stays inlined (the 0431 inlining trap), checked with `bench/count.py`.

**Done when.** Both parity cases match in narrow and wide templates and their lines are gone.
