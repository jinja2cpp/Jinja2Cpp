---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/643
priority: low
area: parity
depends: [0196]
touches: [src/statements.cpp#LoopState, src/internal_value.cpp#ListConverter]
shares: [test/parity/divergences/statements.txt]
---
# Iterating `loop` takes the rest of the loop

**Problem.** 0196 made `loop` sized (`loop|length`) and gave it Jinja2's repr, but iterating
it still yields the names of its attributes. Jinja2's `LoopContext.__iter__` returns the loop
itself: each item it yields is a pair `(item, loop)` taken from the loop's own iterator, so
`{% for x in [1, 2, 3] %}{{ loop|list }};{% endfor %}` prints
`[(2, <LoopContext 3/3>), (3, <LoopContext 3/3>)];` once and the outer loop ends there
(`statements.loop_iter_list`, `statements.loop_iter_for`). Filters that take one item
(`loop|first`) advance the loop by one. The map behind `loop` also still answers
`dictsort` and `items` (`statements.loop_dictsort`), where Jinja2 raises
`AttributeError: 'LoopContext' object has no attribute 'items'`.

Two repr gaps sit next to it, found by the 0188/0196 verifier: `loop|pprint` prints the
attribute map, where Jinja2 prints `<LoopContext 1/3>`, and `loop.cycle|string` prints
nothing, where Jinja2 prints `<bound method LoopContext.cycle of <LoopContext 1/3>>`
(`statements.loop_pprint`).

**Proposal.** Give the loop accessor an item sequence that the list conversion uses ahead of
the map keys: an enumerator that advances the shared `LoopState` (index0, the item ring,
`isLast`) and yields `(item, loop)`, with the running `for` statement leaving once the state
says the items are taken. The mapping filters (`dictsort`, `items`) should refuse it as a
non-mapping, and pprint print the loop as the plain repr does. Low value: no template in the wild iterates `loop`; do it when the loop code is
open for something else.

**Done when.** No line of `test/parity/divergences/` names 0219.

**Resolution.** `loop` is an object (`GetObjectClass` is `LoopContext`) that iterates:
`IMapAccessor::GetIteration` gives a generator of `(item, loop)` tuples that advances the
shared LoopState, and both loop renderers go on after the items the body took. Its object
class makes `dictsort` and `items` refuse it and `pprint` print its repr; `is iterable`,
`list`, unpacking and every filter that converts to a list iterate it. Left: a for loop over
`loop` fetches one item early (0304), and `loop.cycle|string` prints nothing, a callable repr
gap that belongs with 0214 (`statements.loop_cycle_string`).
