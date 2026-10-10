---
status: open
priority: low
area: parity
depends: []
touches: [src/statements.h, src/statements.cpp, src/template_parser.cpp]
---
# `from ... import` of one name under two names

**Problem.** `{% from 'm' import a, a as b %}` binds both `a` and `b` in Jinja2
(`{{ a }}{{ b }}` gives `77` when `m` sets `a = 7`). Jinja2C++ keeps one target per
imported name, the last alias, so `a` stays undefined and the output is `7`. Master kept
the names in a map from name to alias; 0118 P5b-3c moved them into the tree as a list and
kept that result on purpose (test `NodeArenaTest.LongNamesFromTheTree`).

**Proposal.** Keep every `name as alias` pair in source order: the parser stops merging
pairs by name, `ImportStatement::ImportNames` binds each one. A name repeated with the same
alias stays harmless. Add a corpus case under `test/parity/cases/` and update the expected
result in the test above.

**Done when** the case renders as in Jinja2.
