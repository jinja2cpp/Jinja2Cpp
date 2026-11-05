---
status: open
priority: low
area: api
depends: [0358]
touches: [src/template_parser.cpp]
---
# Warn about a special token fragment written next to `{{ }}` in prompt mode

Filed by the 0358/0359 plan (Ruslan 2026-11-05).

**Problem.** The prompt mode guarantee (docs/cpp-only-filters.md) leaves one hole: template
text or a literal that ends with a proper prefix of a special right before `{{ }}`, or starts with
a proper suffix right after it (`<|{{ role }}|>`), can form a special with a clean value, as
`<scr{{ x }}` does in HTML.

**Proposal.** In prompt mode, a load-time check of the text around each expression against the
environment's patterns: a warning first (through the error handler or a load diagnostic), an error
under an option.

**Done when.** Load tests show `<|{{ role }}|>` is reported and `<|im_start|>{{ role }}` is not.
