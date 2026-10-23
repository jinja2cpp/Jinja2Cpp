---
status: open
priority: low
area: ci
touches: [.github/workflows/codeql-analysis.yml]
---
# Find what in the test suite makes CodeQL's data-flow stage explode

**Problem.** The CodeQL job analysed the library and `jinja2cpp_tests` together. Its
"Perform CodeQL Analysis" step on master took 15 min at 7c838c0 (2026-10-18), then 41 min
after train 24 (523f7ab), 73 after train 25 (3985100) and 122 after train 26 (941c91c),
while the build step stayed at 9-11 min. On PR #617 (2026-10-23) 19 of the 61 default
queries (DoubleFree, UseAfterFree, the CWE taint queries) all finished together after
103-109 min: they wait on one shared data-flow stage, not on their own logic. The same
query pack (cpp-queries 1.9.2, bundle 2.27.3) on a library-only database takes 4.2 min on
a 4-core, 16 GB machine. The source added in those trains is a few thousand lines, so the
growth is superlinear in something the tests do (the many-alternative `Value`/`InternalValue`
variants and their visitors instantiated from hundreds of `TEST`/`MULTISTR_TEST` bodies are
the first suspect, not a measured cause).

The fix in this PR analyses the library alone on pushes and PRs, and the tests only in the
weekly (and manual) run under the `with-tests` category. That run still takes about two
hours and will keep growing toward its 240-minute timeout.

**Proposal.** Build the with-tests database locally (see the PR for commands), run one slow
query (`Critical/UseAfterFree.ql`) with `--evaluator-log` and `codeql generate log-summary`,
and name the predicates and source files that dominate. Then either change the test code
that drives it (if it is an accident, e.g. one huge generated function), exclude that test
file from the with-tests build, or accept the cost and record why.

**Done when** the weekly with-tests run is under 60 minutes, or this file records the
measured cause and why it is accepted.
