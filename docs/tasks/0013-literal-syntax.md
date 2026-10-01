---
status: done
priority: high
area: parity
depends: [0001]
touches: [src/lexer.cpp, src/lexer.h, src/lexertk.h, src/expression_parser.cpp, src/expression_parser.h, src/expression_evaluator.h#DictCreator, src/expression_evaluator.cpp#DictCreator, test/errors_test.cpp]
shares: [src/template_parser.h, src/value_visitors.h]
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

**Scheduling.** `none` should evaluate to the None value; until 0034 lands that is `EmptyValue`, so use the existing constructor and let 0034 switch it.

**Done when.** No line of `test/parity/divergences.txt` names task 0013, and `ctest -R parity` passes.

**Next.** Unblocks the `xmlattr`, `items`, `tojson(indent)` and `urlencode` cases, which
can only then show whether the filters themselves match.

**Outcome.** The lexer reads `0x`/`0o`/`0b` prefixes and `_` separators and no longer
reads a stale `errno` (which turned every integer literal after an overflowing float
into a float). The parser accepts `none`/`None`, `()`, `(x,)`, trailing commas in
lists, tuples and dicts, `{key: value}` with any key expression next to the old
`{'key' = value}`, and adjacent string literals. Newly reachable `x // 0` on integers
trapped (SIGFPE); it now takes the float path like `/` until 0015 raises the Python
error. What the literals unblocked is now listed under the task that owns the rest:
printing (0012), `None` (0034), filters (0018, 0019), `}}` inside a tag (0028) and
non-string keys (0036).
