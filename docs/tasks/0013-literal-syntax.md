---
status: open
priority: high
area: parity
depends: [0001]
touches: [src/lexer.cpp, src/lexer.h, src/lexertk.h, src/expression_parser.cpp, src/expression_parser.h, test/parity/]
---
# Literal syntax: `none`, numeric forms, dict and tuple literals

**Problem.** Templates written for Jinja2 do not load in Jinja2C++ because basic literals
fail to parse: `none`/`None` (the lexer has the keyword, the expression parser does not
accept it), `{'a': 1}` (only the non-standard `{'a' = 1}` parses), `(1,)` and `()`,
`[1, 2,]`, `1_000`, `0x1F`, `0o17`, `0b101`, adjacent string literals `'a' 'b'`, and dict
keys that are not strings. A parse error rejects the whole template, so one literal
blocks everything else in it (28 cases).

**Proposal.** Extend the lexer's number scanning (prefixes, underscores) and the primary
expression parser (`none`, tuple forms, trailing commas, `:` in dict literals, any
expression as a dict key, implicit string concatenation). Keep `{'a' = 1}` working as a
documented C++ extension. Dict keys that are not strings need the value model to allow
them; if that is too large, accept integer keys by converting them and record the gap.

**Done when.** No line of `test/parity/divergences.txt` names task 0013, and `ctest -R parity` passes.

**Next.** Unblocks the `xmlattr`, `items`, `tojson(indent)` and `urlencode` cases, which
can only then show whether the filters themselves match.
