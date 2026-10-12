---
status: done
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
match (their lines go from `test/parity/divergences/statements.txt`), `globals.int_called_in_loop`
matches (its line goes from `globals.txt`; folded in from 0042, which now covers only the
"globals are maps" gaps), `{{ loop.cycle }}` no longer prints `2`, the `loop.cycle` unit tests in
`test/forloop_test.cpp` still pass, `loop_cycle`,
`loop_cycle_as_value` and `methods.loop_cycle_no_args` still match, and the two bench cases
move by less than 1%.

Found by 0117 P1a.

**Left over.** A stored `loop.cycle` is bound to the position it was read at (`MakeLoopCycle`
captures `index0`); Jinja2's bound method follows the loop, so a callable stored on iteration 1
and called on later ones cycles from the start (`statements.loop_cycle_stored_called_later`,
listed in `divergences/statements.txt`). Following the loop needs a callable that borrows the
loop frame, which the frame pool (0133) makes unsafe once the loop ends. `loop.cycle('a', x=1)`
as a direct call still ignores keyword arguments (`LoopAttrExpression::TryCallCycle`).
