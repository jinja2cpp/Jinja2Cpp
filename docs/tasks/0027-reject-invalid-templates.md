---
status: done
priority: medium
area: parity
depends: [0001, 0014, 0024]
touches: [src/template_parser.cpp#splitter]
shares: [src/template_parser.cpp, src/template_parser.h, src/expression_parser.cpp, src/filters.cpp]
---
# Reject what Jinja2 rejects

**Problem.** Jinja2C++ renders templates that Jinja2 refuses: unclosed `{{`, `{%` blocks
and `{#` comments at end of input, `else` after `else`, `elif` after `else`,
`{% set a %}` without a body, unpacking count mismatches, filters called with arguments
of the wrong type or unknown keywords, `sort` over mixed types (13 cases; related type
errors are in 0015, 0017, 0022, 0023). Accepting a broken template hides the author's
mistake and makes behaviour depend on parser details.

**Proposal.** Make each a parse or render error with Jinja2's wording where practical.
Only the fact of an error is compared by the corpus; message parity is a later step.

**Scheduling.** Unclosed `{{`/`{%`/`{#` at end of input are in the template splitter that 0024 changes.

**Done when.** No line of `test/parity/divergences/` names task 0027, and `ctest -R parity` passes.

**Done in #312.** The 13 cases fail as in Jinja2; see the PR for what else moved (tuple unpacking,
filter argument checks, ordering errors) and task 0045 for `None`/undefined ordering.

**Next.** Compare error messages and line numbers too, behind a separate corpus field,
once the error cases agree on failing.
