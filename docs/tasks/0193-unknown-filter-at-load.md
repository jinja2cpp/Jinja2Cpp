---
status: open
priority: low
area: parity
depends: []
touches: [src/template_parser.cpp, src/expression_parser.cpp]
shares: [test/parity/divergences/filters.txt]
---
# An unknown filter is a load error

**Problem.** Jinja2 reports a filter it does not know when the template is compiled
(`TemplateAssertionError: No filter named 'x'`), except inside an `if` branch that is never
taken, where the check is deferred to runtime. Jinja2C++ reports it only when the filter
runs, so a macro that is never called, or a default that is never needed, renders without
an error (`filters.unknown_filter_in_uncalled_macro`, `filters.unknown_filter_in_unused_default`).
0166 made the second case visible: defaults are now evaluated per call, as in Jinja2, so an
unused default is never evaluated. Found by the 0166 verifier.

**Proposal.** Resolve filter names at load against the built-ins and the environment's
filters, and keep Jinja2's deferral for code under a constant-false `if`. User filters
added to the environment after the template loads need a decision (Jinja2 binds at
compile time too). Tests have the same gap (0017's "Left over").

**Done when.** No line of `test/parity/divergences/` names 0193.
