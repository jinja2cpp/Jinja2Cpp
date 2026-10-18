---
status: done
priority: high
area: api
depends: []
touches: [src/statements.cpp, src/expression_evaluator.cpp]
---
# A user callable taking `*context` gets None for a positional argument

**Release triage (0221).** Host-callable argument binding is a core embedding
contract. Confirmed on master 66b9096: next to the keyword row at
test/user_callable_test.cpp:342, `ContextArgFn('test_value')` renders `none` (narrow
and wide) where `ContextArgFn(name='test_value')` renders `100500`; that row is the
reproducer. Fix the binding defect before release. Include the supported
behavior in the host-callable integration scenarios of 0223.

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

**Resolution.** Argument binding (`MapKeywordArgs` in src/expression_evaluator.cpp)
treated `*context` as an ordinary named parameter, so the first positional argument
was bound to it and then shadowed by the engine-supplied context, leaving `name`
unbound. `*context` now gets its own `Context` state: no call argument binds to it,
and the positional scan steps over it wherever it is declared. The scan also used to
follow the missing arguments from the declaration's first slot rather than the first one
it had not skipped, which dropped the second positional of `[*context, a, b]`. Rows in the
`GlobalContextAccess` table of test/user_callable_test.cpp cover positional and
keyword calls with `*context` first (one and two named parameters) and in the middle, plus a keyword call without it.
