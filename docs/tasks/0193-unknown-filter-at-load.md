---
status: done
priority: low
area: parity
depends: []
touches: [src/template_parser.cpp, src/template_parser.h, src/expression_parser.cpp, src/expression_parser.h]
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

**Done.** The parsers report each name that is neither built in nor added to the
environment; the template parser drops those Jinja2 defers (an `if` statement, its tests and
branches, and an inline if, up to the next statement with a frame of its own: a loop, its
filter and its `else`, a macro and its defaults, a call block's parameters, a block, `with`
bodies, filter and set blocks, `autoescape`), and once the template has parsed reports the
first left as `ErrorCode::UnknownFilter` or `UnknownTest`, root body before blocks, as Jinja2
compiles them. Tests are covered too (0017's left over). Ruslan chose (2026-10-16) Jinja2's
behaviour by default plus `Settings::userCallablesAsFilters`, which keeps 1.x's lookup of
filters and tests among the render parameters and globals when they run; without it that
lookup, and `is filter`/`is test` counting such values, are gone. As in Jinja2, output tags
(`{{ }}`, `trans`) after a top-level `extends` are not checked unless inside a macro, call
block, set block or block; Jinja2C++ still evaluates that output when it renders (0203).
Deliberate: with several unknown names the one reported follows the source, where Jinja2
compiles a filter or set block's body before its header, a call block's body before its
arguments, and a loop's test and body before its iterable; both reject.
