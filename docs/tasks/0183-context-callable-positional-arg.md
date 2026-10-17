---
status: open
priority: high
area: api
depends: []
touches: [src/statements.cpp, src/expression_evaluator.cpp]
---
# A user callable taking `*context` gets None for a positional argument

**Release triage (0212).** Host-callable argument binding is a core embedding
contract. This report is not yet a confirmed minimal reproducer: reproduce it first,
then fix the binding defect or close the report with evidence. Include the supported
behavior in the host-callable integration scenarios of 0214.

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
