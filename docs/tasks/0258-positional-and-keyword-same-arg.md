---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/629
priority: low
area: api
depends: []
touches: [src/expression_evaluator.cpp, src/expression_evaluator.h, src/function_base.h, src/internal_value.cpp#PrepareUserCallableParams, test/user_callable_test.cpp, test/parity/cases/errors.py]
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

**As built.** `CheckMultipleValues` (expression_evaluator.cpp) runs after the positional
arguments are mapped: a parameter given by keyword that a positional argument passed over, or any
parameter before `*args` when positional arguments are left over, is the call's
`multipleValues`. A user callable then fails with "got multiple values for argument 'a'"
(TemplateRuntimeError), and a filter or test bound at Load reports the same message as its
argument error. Parameters after `*args` take keywords only, as in Python. Macros needed no
change: Python Jinja2 reports `m('2', '3', a='1')` as "macro 'm' takes no keyword argument 'a'"
(checked with Jinja2 3.1.6), which is what Jinja2C++ already says, so the proposal's "route
macros through the same check" would have moved them away from Python. Corpus:
`errors.macro_positional_and_keyword`, `errors.filter_positional_and_keyword`.

**Closed (2026-10-24).** Merged in #629 (train 31, #633).
