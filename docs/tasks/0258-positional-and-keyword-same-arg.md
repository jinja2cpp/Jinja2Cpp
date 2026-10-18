---
status: open
priority: low
area: api
depends: []
touches: [src/expression_evaluator.cpp, test/user_callable_test.cpp]
---
# A call that gives one argument both positionally and by keyword is accepted

**Problem.** Found by the verifier on 0183. For a user callable declared
`[a, b, c]`, `f('2', '3', a='1')` renders with `a='1', b='2', c='3'`; Python raises
`TypeError: f() got multiple values for argument 'a'`. `ParseCallParamsImpl` binds
keywords first and then fills the remaining slots from the positionals, so the
positional that belongs to `a` silently moves to the next missing slot. Macros reject
the same call, but with a different message ("takes no keyword argument 'a'").

**Proposal.** When positional argument `i` would land on a slot after the one it
belongs to because that slot was given by keyword, fail the call with Python's
"got multiple values for argument" error; route macros through the same check so the
message matches Python.

**Done when.** user_callable_test.cpp and a macro test reject `f('2', '3', a='1')`,
and the parity corpus has a case for the macro form.
