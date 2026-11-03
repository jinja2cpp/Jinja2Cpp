---
status: done
priority: low
area: parity
depends: [0305]
touches: [src/statements.cpp#LoopCycleCell]
---
# A kept `loop.cycle` prints `?` for the length of a lazily filtered loop it broke out of

**Problem.** Found doing 0305. A `loop.cycle` or `loop.changed` callable prints as Jinja2's bound
method, `<bound method LoopContext.cycle of <LoopContext 1/2>>`. The callables share a cell with
their loop (`LoopCycleCell`), not the loop itself: a callable that kept the loop would make the
loop collect the rest of its items when it ends (the `use_count` check in `ForStatement`), which
runs the loop filter for items Jinja2 never filters (`statements.loop_cycle_stored_filtered_break`
raises in the filter then). So a callable kept past a `break` out of a loop whose length was not
known yet, a lazily filtered one (`for x in l if ...`), has no items left to count, and prints the
length as `?`. Jinja2 counts them when the repr is taken, which also runs the filter then (and
raises there if the filter does). Pinned by `statements.loop_cycle_repr_after_break_lazy`.

**Proposal.** Keep what the loop needs to fetch its rest (its enumerator over a list the cell
keeps alive, and the filter's expression with a context of its own) in the cell when a callable
shares it at the loop's end, and collect the rest on the first repr. Only worth it if a template
in the wild prints such a callable; otherwise mark the line deliberate.

**Done when** the line is gone from `test/parity/divergences/statements.txt` or marked deliberate.

**Decision (Ruslan, 2026-11-03).** Deliberate. Counting the rest at repr time would keep the
loop's enumerator, its list, the filter's expression and the names the filter reads alive past
the loop statement and its frame (a closure capture of its own, against the 0188 `use_count`
check and the cycle collector), for the repr of a bound method kept past a `break` and printed.
The divergence line stays, marked deliberate.
