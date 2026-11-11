---
status: in-progress
priority: low
area: parity
depends: []
touches: [src/expression_parser.cpp, src/expressions.cpp]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt, python/tests/test_fallback.py]
---
# A subscript of a literal by a tuple of slices is a parse error in the engine

**Problem.** Jinja2 3.1.6 parses a subscript whose argument is a tuple holding a slice
(`Parser.parse_subscript`: `x[1:2, 3]`, `x[:, :]`), Python's extended slice. What it does next
depends on the subscripted value:

- a literal is folded at compile time through `Environment.getitem`, which catches the TypeError
  of a list, dict or string and gives an undefined: `{{ [1, 2][1:2, 3] }}` renders empty, and
  `{% if false %}{{ ()[:, :] }}{% endif %}` loads;
- any other value (`{{ l[1:2, 3] }}`) compiles to invalid Python: `from_string` raises
  SyntaxError, a load error as the engine's.

Jinja2C++ reports `']' expected` for both, so it differs only for a literal
(`subscripts.tuple_of_slices_literal` in the parity corpus; `tuple_of_slices_name` matches).

The Python module's tests use the literal form as the syntax the engine lacks, to send a
template to Jinja2 (`NOPARSE` in `python/tests/test_fallback.py`, `test_candidate_is_unparsable`;
also `test_patch_hands_unloadable_templates_to_transformers`). Found by 0419, which made
`{% print %}`, the previous sample, parse.

**Proposal.** Low value on its own: parse a tuple of slices in a subscript of a literal and fold
it to the undefined Jinja2 gives; keep a load error for other values. Then give `NOPARSE`
another syntax the engine lacks, or drop the tests that need one.

**Done when.** `subscripts.tuple_of_slices_literal` matches in narrow and wide templates and its
divergence line is gone.
