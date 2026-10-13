---
status: open
priority: medium
area: api
depends: []
touches: [src/statements.cpp, src/expression_evaluator.cpp]
---
# A user callable taking `*context` gets None for a positional argument

**Problem.** Found while building 0165: a user callable declared with
`ArgInfo{"*context"}` plus a named parameter receives None when the template passes that
parameter positionally; passing it by keyword works. Python's `@pass_context` functions
take their arguments positionally like any other function. Not yet reduced to a minimal
reproduction or traced to the argument-binding code.

**Proposal.** Reproduce in test/user_callable_test.cpp (`{{ f(1) }}` against
`{{ f(x=1) }}` with a `*context` callable), find where the `*context` slot shifts or
drops positional binding, and fix it there.

**Done when.** The new user_callable_test.cpp rows pass for positional and keyword
arguments, with and without `*context`.
