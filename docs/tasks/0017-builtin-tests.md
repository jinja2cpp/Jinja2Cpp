---
status: open
priority: medium
area: parity
depends: [0001]
touches: [src/testers.cpp, src/testers.h, src/expression_parser.cpp]
---
# Complete the builtin tests

**Problem.** Jinja2 ships 39 tests; Jinja2C++ lacks `boolean`, `callable`, `divisibleby`,
`escaped`, `false`, `true`, `filter`, `test`, `float`, `integer`, `none` and `sameas`. An
unknown test silently evaluates to false instead of failing at load time, so templates
using a missing test render wrong output with no error. `x is odd and y is float` also
groups differently (12 cases).

**Proposal.** Add the missing testers in `src/testers.cpp`, make an unknown test name a
template error like an unknown filter (Jinja2 defers it to runtime only inside a branch
that is never taken), and fix the precedence of `is` relative to `and`/`or`. The parser
part (`is not`, keyword names, arguments without parentheses) is task 0014.

**Done when.** No line of `test/parity/divergences.txt` names task 0017, and `ctest -R parity` passes.
