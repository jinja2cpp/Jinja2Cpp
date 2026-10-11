---
status: open
priority: low
area: parity
touches: [src/lexer.h, test/parity/cases/errors.py, test/parity/divergences/errors.txt]
---
# The tag lexer accepts two inputs Jinja2 rejects

**Problem.** Found while porting lexertk into `TagLexer` (0142), which keeps both
behaviours so that 0142 changes no output:

- `;` ends the tokens of a tag and the rest of the tag is ignored: `{{ x ; 1 }}` renders `3`.
  lexertk read `;` as its end-of-input token. Jinja2: "expected token 'end of print
  statement', got ';'". Pinned by `errors.semicolon_in_tag`.
- A decimal literal takes a second exponent: `{{ 1e1e-1 }}` lexes as one number and
  renders `9.0`. Jinja2 lexes `1e1` and then fails on the name `e`. Pinned by
  `errors.float_second_exponent`.

**Proposal.** In `TagLexer::ScanToken`, make `;` a lexing error like any other unknown
character (then `LexToEnd` no longer has to fall back on it), and in `DecimalStep` stop
the literal at an `e` once it has an exponent. Delete the two lines from
`test/parity/divergences/errors.txt`.

**Done when** both cases match Python and the corpus is otherwise unchanged.
