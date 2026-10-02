---
status: open
priority: medium
area: parity
depends: [0001, 0014, 0034]
touches: [src/testers.cpp, src/testers.h]
shares: [src/expression_parser.cpp]
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

**Scheduling.** `none`, `true`, `false` tests need 0034's None value; the parser half and the `is` precedence are 0014's.

The `none` test landed with 0034 (#313).

**Done when.** No line of `test/parity/divergences/` names task 0017, and `ctest -R parity` passes.
