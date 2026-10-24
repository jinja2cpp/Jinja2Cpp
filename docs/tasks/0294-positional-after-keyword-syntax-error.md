---
status: in-progress
priority: low
area: parity
depends: [0258]
touches: [src/expression_parser.cpp, src/filters.cpp#ApplyMacro, test/filters_test.cpp, test/user_callable_test.cpp, test/parity/divergences/errors.txt, docs/cpp-only-filters.md]
---
# A positional argument after a keyword one is a syntax error

**Problem.** Found while doing 0258. Jinja2 rejects `f(a=1, 2)` at load with TemplateSyntaxError
"invalid syntax for function call expression"; Jinja2C++ parses it
(`ExpressionParser::ParseCallParams` keeps positional and keyword arguments in separate lists and
forgets their order) and binds the keywords first. The C++-only `applymacro` filter documents
this form (`applymacro(macro='name', ...)`, docs/cpp-only-filters.md and the site's
usage/macros.md), and `test/filters_test.cpp` `ApplyMacroTest` and the `joiner` rows of
`test/user_callable_test.cpp` use it. Pinned by `errors.call_positional_after_keyword`
(`accepts`).

**Proposal.** Fail the parse with Jinja2's message when a positional argument follows a keyword
one. `applymacro` keeps working when written the Python way: `macro` is keyword-only (it takes
extra positional arguments without declaring `*args`, so 0258's check already treats it so), so
`applymacro('x', macro='name')` binds as before; rewrite the documented examples and the test
rows in that order. A breaking change for templates that use the old order: list it in the 2.0
migration notes (0077).

**Done when** the corpus line is deleted, the C++ tests use the Python order, and the docs show it.
