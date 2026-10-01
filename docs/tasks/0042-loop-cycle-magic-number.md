---
status: open
priority: low
area: parity
depends: [0030]
touches: [src/statements.cpp#ForStatement, src/expression_evaluator.cpp#CallExpression]
---
# `loop.cycle` is the integer 2, so calling any 2 cycles

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

**Done when.** `LoopCycleFn` is gone, `globals.int_called_in_loop` no longer names this
task in `test/parity/divergences.txt` (it may move to 0026), and the `loop.cycle` unit
tests in `test/forloop_test.cpp` still pass.
