---
status: open
priority: low
area: parity
depends: [0220]
touches: [src/function_base.h, src/expression_parser.cpp#ParseCallParams, src/filters.cpp, src/testers.cpp, src/filter_factories.cpp, src/tester_factories.cpp]
shares: [test/parity/divergences/calls.txt]
---
# `*args` and `**kwargs` in filter and test calls

**Problem.** 0220 made calls unpack `*args` and `**kwargs` when they are evaluated. Jinja2 takes
them in filters and tests as well (`'a'|replace(*['a', 'b'])` renders `b`,
`4 is divisibleby(*[2])` is True, `{% filter replace(*l) %}` too), but Jinja2C++ binds filter
and test arguments to their parameters at Load (`FunctionBase::ParseParams`, `BoundArguments`
hold one expression node per parameter), and an unpacked argument is only known at render. So
`ExpressionParser::ParseCallParams` still rejects `*` and `**` there with `Unexpected token`.
Pinned by `calls.filter_star_args`, `calls.filter_star_kwargs`, `calls.test_star_args` and
`calls.filter_block_star_args`.

**Proposal.** Keep the Load-time binding for the common call and add a render-time one for a
call that unpacks: `BoundArguments` holding values instead of nodes, bound per evaluation from
the spliced `CallParams` (as `CallExpression` does through `helpers::EvaluateCallParams`). The
extras `map`, `select` and `applymacro` forward (`CallParamsInfo* extras`) must carry values in
that mode too. Only the unpacking call pays; check `bench/count.py` for the common one.

**Done when.** No line of `test/parity/divergences/` names 0306.
