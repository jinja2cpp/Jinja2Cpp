---
status: open
priority: low
area: parity
depends: [0030]
touches: [src/statements.cpp#ForStatement, src/expression_evaluator.cpp#CallExpression, src/global_functions.cpp]
---
# Global function follow-ups: `loop.cycle` is the integer 2, globals are maps

**Problem.** `CallExpression::Evaluate` dispatches on the integer value of the callee:
`loop.cycle` is stored as `LoopCycleFn` (the integer `2`), and any value that converts to
`2` is treated as that function. Inside a loop, `{% set f = 2 %}{{ f('a', 'b') }}` prints
`a` where Jinja2 raises `TypeError: 'int' object is not callable` (`globals.int_called_in_loop`),
and `{{ loop.cycle }}` prints `2`. `range` had the same scheme (`RangeFn = 1`) until 0030
made it a `Callable`.

**Proposal.** Make `loop.cycle` a `Callable` that captures the loop's index (as
the recursive `loop()` already is) and delete the `LoopCycleFn` enum and the integer
dispatch in `CallExpression::Evaluate`. Calling a non-callable then reaches
`CallArbitraryFn`, which renders empty; raising there instead is 0026's concern.

**More gaps found by the 0030 review** (same area, fix together or split out):
- `cycler(...)` and `joiner(...)` return `MapAdapter`s of callables, so `c is mapping` is
  true (`globals.cycler_not_mapping`), `c|length` is 5 and `{{ c }}` prints their internals,
  where Jinja2 has opaque objects (`<jinja2.utils.Cycler object at ...>`). An object kind
  that is subscriptable but neither a mapping nor iterable would fix all three.
- `range()` converts its arguments with `ConvertToInt`, so `range(1.5)` and `range('3')`
  render where Python raises `TypeError` (`globals.range_float_argument`); `dict(none)`
  renders `{}` (that one also needs 0034 to tell `None` from undefined).

**Done when.** `LoopCycleFn` is gone, no line of `test/parity/divergences.txt` names 0042
(`globals.int_called_in_loop` may move to 0026), and the `loop.cycle` unit tests in
`test/forloop_test.cpp` still pass.
