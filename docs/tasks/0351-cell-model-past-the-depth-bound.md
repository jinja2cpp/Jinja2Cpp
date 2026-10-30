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
7-minute probe stopped it. Nothing but size blocks a fixed point: the
epoch advances only when a cell is adopted or a frozen render lists cells (at most `2*MaxCell`),
the call id only at a module call's start (at most `MaxAd/2`), and failures and values are
bounded, so every scope is finite without the level guard (0338 step F's count, against
master after #695; recheck once #709's failure steps land). It is still large: `module` with
`MaxAd=6`, `MaxCell=2`, `MaxGen=3`, `MaxEx=1` had not finished after 25 minutes, so the runs
need the 340-minute job and the smallest scopes first. Add a run kind
`exhaustive` whose verdict requires TLC's "No error" without the level guard firing, and run
`module`, `module-0332`, `module-generic` and `threads-module` that way in the weekly job.

**2. Simulation past the bound.** `tlc2.TLC -simulate -depth 40` samples random behaviours far
deeper than the BFS reaches. Not a proof, but it finds deep counterexamples (the loop and sort
bugs needed about 14 steps) in every scope, including the full-action ones. Add `--simulate
SECONDS` to check.py and a weekly shard that gives each pass run a fixed budget.

**3. Inductive invariants: owned by 0338 step F**, in its own thread and task: Apalache
typing of `CellProtocol.tla`, `Close` as a fold, a wrapper module without the level guard,
`IndInv /\ Next => IndInv'`, and the cutoff argument. This task does not touch it.

Alloy 6 is the alternative for the heap half only: GC reachability, ownership links and the
registry are relations with transitive closure, which Alloy states directly and checks for all
heaps up to N atoms. It would be a second model to keep in step with the C++, outside the trace
validation of 0338 step B, so it is worth it only if Apalache cannot handle `Close`.

**Done when** the README states, per run, whether it is depth-bounded or exhaustive for its
constants, and the weekly job runs the exhaustive and simulation kinds.
