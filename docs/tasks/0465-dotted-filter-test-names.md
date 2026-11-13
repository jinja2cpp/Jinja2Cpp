---
status: in-progress
priority: low
area: parity
depends: [0095]
touches: [src/expression_parser.cpp, test/parity/divergences/fuzz.txt, python/tests/parity/module-divergences.txt]
---
# Dotted filter and test names (`x|ns.f`, `x is ns.t`)

**Problem.** Jinja2's parser reads a filter or test name as a dotted path: `parse_filter` and
`parse_test` append `.name` while a dot follows, so `x|first.upper()` is the filter
`first.upper` called with no arguments, and a host can register `env.filters['my.f']` and
write `x|my.f`. An unknown filter or test inside a branch that does not run is no error in
Jinja2 3.1 (it fails only when it runs). Jinja2C++ stops the name at the dot and then fails
the load, so `{% if false %}{{ x|first.upper() }}{% endif %}ok` renders `ok` in Python and
is rejected here. The generated templates of the differential check (docs/tasks/0095) found
it, from `str_expr|filter.method()` chains.

**Proposal.** Read dotted names in the filter and test parsers of
`src/expression_parser.cpp`, and look them up as one name, so a dotted host filter or test
(`TemplateEnv::AddFilter("my.f", ...)`) works. Keep 0193's rule for unknown names (an
unknown name outside a conditional branch is a load error), and check that `x|f.g` with an existing `f` does not silently
become `(x|f).g`: Python rejects that as the unknown filter `f.g`.

**Done when.** `fuzz.dotted_filter_name_not_run` and `fuzz.dotted_test_name_not_run` match
and their lines in `test/parity/divergences/fuzz.txt` and
`python/tests/parity/module-divergences.txt` are deleted, and a unit test renders
a dotted host filter and test.
