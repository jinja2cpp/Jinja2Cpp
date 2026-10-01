---
status: open
priority: low
area: parity
depends: [0024]
touches: [src/template_parser.h#StripBlockLeft]
shares: [src/template_parser.h, test/parity/divergences.txt]
---
# lstrip_blocks and modifier leftovers

**Problem.** Found by the verifier on PR #304 (task 0024), all present before it:

- With `lstrip_blocks`, trailing whitespace at the end of a template is dropped: `"a\n  "`
  renders `a\n`, `"{{ 1 }}  "` renders `1` (`lstrip_trailing_whitespace`,
  `lstrip_expression_trailing_space`). `FinishCurrentBlock(size, RawText)` at the end of
  `DoRoughParsing` runs `StripBlockLeft` as if a block followed.
- With `lstrip_blocks`, whitespace-only text between two tags on one line is stripped:
  `{% endif %} {% if true %}` loses the space (`lstrip_between_tags`). Jinja2 strips
  only when the text before the tag on that line is whitespace back to the line start.
- `-` strips only what `std::isspace` (C locale) calls whitespace; Python's `\s` also
  matches U+00A0, U+2003 and other Unicode spaces (`minus_strips_unicode_space`).
- `{% raw +%}` is accepted; Jinja2's raw regex allows only `-%}` or `%}` and rejects it
  (`raw_plus_close_rejected`).

**Proposal.** Make `StripBlockLeft` require that only whitespace precedes the tag back to
a newline or the start of the template, and skip it when no tag follows. Use a Unicode
whitespace test for `-` stripping (narrow input is UTF-8, so decode before testing).
Reject `+` before the `%}` of `raw`.

**Done when.** No line of `test/parity/divergences.txt` names task 0044.
