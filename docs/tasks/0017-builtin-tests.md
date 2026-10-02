---
status: done
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

**Outcome.** All 39 Jinja2 tests exist; `x is odd and y is float` now matches. Left over:
- An unknown test is a render-time error ("No test named 'x'."), not a load-time one,
  because tests registered as user callables are only known once the template renders. A
  branch never taken therefore never fails, as in Jinja2.
- `escaped` is always false until strings carry a markup flag (0025 owns
  `tests.escaped` and `autoescape.escaped_test`).
- `sameas` compares scalars by kind and value (CPython caches small ints and interns
  literals) and lists or dicts by the container they view. Containers built in a template
  are copied on assignment, so `{% set b = a %}{{ b is sameas a }}` is false until 0020
  gives them reference semantics (`tests.sameas_container`).
