---
status: done
priority: low
area: parity
depends: [0144]
touches: [src/statements.cpp#LoopAccessor, src/expression_evaluator.cpp#LoopAttrExpression]
shares: [test/parity/divergences/]
---
# A stored `loop.cycle` keeps the position it was read at

**Problem.** 0144 made `loop.cycle` a callable, but `MakeLoopCycle` captures `index0` when the
attribute is read. Jinja2's `loop.cycle` is a bound method that reads the loop's current
position at call time, so a callable stored on the first iteration and called on later ones
cycles from where the loop is, not from where it was read
(`statements.loop_cycle_stored_called_later`, the one line in
`test/parity/divergences/statements.txt` that still named the closed task 0144). Following
the loop needs a callable that refers to the loop frame, which the frame pool (0133) reuses
once the loop ends. `loop.cycle('a', x=1)` called directly also still ignores keyword
arguments (`LoopAttrExpression::TryCallCycle`), where Jinja2 raises `TypeError`.

**Proposal.** Bind the stored callable to the loop frame through the frame pool's generation
(the same check `loop()` uses since 0165): while the loop runs it reads the live `index0`, and
after the loop ends it raises, or keeps its last position if Jinja2 does the same (check with
the oracle). Reject keyword arguments in `TryCallCycle`.

**Done when.** No line of `test/parity/divergences/` names 0188 and `ctest -R parity` passes;
a corpus case pins the keyword-argument error.
