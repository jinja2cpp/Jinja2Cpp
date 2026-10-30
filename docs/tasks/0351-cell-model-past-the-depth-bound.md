---
status: open
pr:
priority: medium
area: robustness
depends: [0338, 0350]
touches: [tla/check.py, tla/README.md, .github/workflows/tla-model.yml]
---
# Check the cell model past its depth bound

**Problem.** Every pass run of `tla/check.py` stops at a level (`MaxLevel`, 9 to 16), and the
space grows about six times per level (task 0350), so a faster machine buys one level at most.
The README calls the result bounded evidence; 0338 step F plans the inductive half. Three
cheaper steps can come first, and they are not exclusive.

**1. Runs without a depth bound for the small scopes.** Ids are never reused and `MaxAd`,
`MaxCell`, `MaxGen` and `MaxEx` bound the allocations, so a scope whose steps all allocate
reaches a fixed point: TLC then reports a complete search with no level bound, a check of every
behaviour for those constants, not only the short ones. Observed: `module` with `MaxLevel=100`
grew 1.3M, 2.3M, 4.2M states at levels 12-14 (slowing, about 1.7 times a level) before a
7-minute probe stopped it. What blocks a fixed point: the epoch and the call id climb to 1000
without an allocation, so they need a wrap (`MaxEpoch`, `MaxCallId`) that is harmless in the
scope (0341 is, under FixCallPin; 0342 is not, so the render scopes need 0342 fixed first, or an
abstraction of the epoch to "equal to the adapter's seen value or not"). Add a run kind
`exhaustive` whose verdict requires TLC's "No error" without the level guard firing, and run
`module`, `module-0332`, `module-generic` and `threads-module` that way in the nightly job.

**2. Simulation past the bound.** `tlc2.TLC -simulate -depth 40` samples random behaviours far
deeper than the BFS reaches. Not a proof, but it finds deep counterexamples (the loop and sort
bugs needed about 14 steps) in every scope, including the full-action ones. Add `--simulate
SECONDS` to check.py and a nightly shard that gives each pass run a fixed budget.

**3. Inductive invariants (0338 step F).** Write `IndInv` (the invariants plus the structural
facts they need: links point at live cells, an attached adapter's cell is in its registry, an
export's storages are alive) and check `IndInv /\ Next => IndInv'` for every state, reachable or
not. Apalache does this symbolically for fixed constants and needs: type annotations on the 25
variables, `Close` (the recursive reachability of GC) as `ApaFoldSet` or a bounded iteration,
and the level guard (`TLCGet("level")`) removed from `Next`. TLC can sample the same check on
random states of `IndInv` when Apalache cannot finish.

Alloy 6 is the alternative for the heap half only: GC reachability, ownership links and the
registry are relations with transitive closure, which Alloy states directly and checks for all
heaps up to N atoms. It would be a second model to keep in step with the C++, outside the trace
validation of 0338 step B, so it is worth it only if Apalache cannot handle `Close`.

**Done when** the README states, per run, whether it is depth-bounded, exhaustive for its
constants, or covered by an inductive check, and the nightly job runs the new kinds.
