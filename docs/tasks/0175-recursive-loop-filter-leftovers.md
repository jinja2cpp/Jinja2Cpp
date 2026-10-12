---
status: open
priority: low
area: parity
depends: [0165]
touches: [src/statements.cpp#RenderLoopInScopes, src/template_parser.cpp#ParseFor]
shares: [test/parity/divergences/statements.txt]
---
# Recursive loop filter: the outer `loop` at depth > 0, and the legacy `recursive if` order

**Problem.** Left by 0091 (PR #500), which made a loop filter see the `loop` around its loop:

- `statements.loop_recursive_filter_sees_outer_loop`: in Jinja2 the filter of a recursive
  loop sees the loop around the recursive `for` at every depth (the recursive loop is a
  closure defined there). Jinja2C++ looks `loop` up when each level starts
  (`ForStatement::RenderLoopInScopes`), and a `loop(...)` call runs where it is called, so
  at depth > 0 the lookup finds the parent level's `loop`. Once 0165 makes `loop()` run
  where the loop is defined, check whether the lookup already finds the right one; if not,
  carry the level-0 value down the recursion (the recursion callable,
  `MakeLoopRecursion`, is 0165's to change).
- `statements.loop_recursive_after_filter_reject_order`: Jinja2 accepts only
  `for x in l if c recursive`. Jinja2C++ took only `recursive if c` before 0091 and now
  takes both, so existing templates keep working. Dropping the old order is a breaking
  change for 2.0: decide whether to reject it (or warn) and update
  `test/arena_switch_test.cpp`, which uses it.

- Not pinned (an include needs a loader case): a template included in a loop sees
  `loop` (`loop is defined` is true there); Jinja2's included template does not.

**Done when.** Both lines are gone from the statements allow-list, or the second one is
kept as a deliberate divergence with that decision noted in docs/parity.md.
