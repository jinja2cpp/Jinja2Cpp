---
status: open
priority: medium
area: parity
touches: [src/statements.cpp#LoopAccessor, src/expression_evaluator.cpp#CallExpression, src/expression_evaluator.h]
---
# `loop.cycle` is the integer 2

**Problem.** `loop.cycle` evaluates to the integer `LoopCycleFn` (2), and
`CallExpression::CallWithCallee` treats a call of any value equal to 2 as `loop.cycle(...)`.
So inside a `for` loop `{% set f = 2 %}{{ f('a', 'b') }}` cycles (`a`, `b`, ...), and outside one
it renders nothing; Jinja2 raises `TypeError: 'int' object is not callable` in both. `CallLoopCycle` then looks `loop` up by name: until
0117 P1a it dereferenced a null pointer when `loop` was not a loop object
(`{% set loop = 1 %}{% set f = 2 %}{{ f('a') }}` crashed); it now renders nothing.
Jinja2 also evaluates every argument of `cycle`, Jinja2C++ only the one it picks.

**Proposal.** Give `loop.cycle` a value of its own: a callable bound to the loop's current
`index0`, like `loop.changed` (`MakeLoopChanged`), and drop `LoopCycleFn`. `for_loop_vars`
and `html_autoescape` call `loop.cycle` per row, so measure them with
`bench/count.py --baseline`: a `Callable` costs an allocation per read today. If it shows,
keep a cheap marker type that cannot be confused with a number (for example a dedicated
alternative in the value variant, which 0140 is reshaping) instead.

**Done when.** `statements.call_int_is_not_loop_cycle` and `statements.call_int_with_int_loop`
match (their lines go from `test/parity/divergences/statements.txt`), `loop_cycle`,
`loop_cycle_as_value` and `methods.loop_cycle_no_args` still match, and the two bench cases
move by less than 1%.

Found by 0117 P1a.
