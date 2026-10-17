---
status: open
priority: low
area: parity
touches: [src/statements_parser.cpp, src/template_parser.h, test/parity/divergences/errors.txt]
---
# An `if` test with a conditional expression, and an unclosed raw block at the end

**Problem.** Found by the verifier on the 0161/0044 PR, both present before it:

- `{% if 1 if x else 2 %}a{% endif %}` is accepted. Jinja2 parses the test of `if` (and
  `elif`) with `parse_tuple(with_condexpr=False)` and fails with "expected token 'end of
  statement block', got 'if'". Pinned by `errors.if_test_conditional_expression`. Line
  statements (`% if 1 if x else 2`) behave the same.
- `{% raw %}` or `{%- raw -%} ` with no `endraw` at the very end of the template is an error
  in Jinja2C++; Jinja2 renders it as empty (its raw regex matches the end of input there).
  `{% raw %} x` with text after the tag is an error in both. Pinned by
  `errors.raw_unclosed_at_end`, `errors.raw_unclosed_minus_space`.

**Proposal.** Parse the `if`/`elif` test without the conditional-expression tail. For raw,
check what exactly Jinja2's `raw_begin` rule accepts at the end of input before matching it.

**Done when** no line of `test/parity/divergences/` names task 0213.
