---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/969
priority: medium
area: robustness
depends: [0338, 0350]
touches: [tla/check.py, tla/README.md, tla/CellProtocol.tla, .github/workflows/tla-model.yml]
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

**Resolution (2026-11-20).** Items 1 and 2 are in `tla/check.py`, the `tla-model` workflow and
`tla/README.md` ("Past the depth bound"):

- Run kind `exhaustive` (`MaxLevel` 1,000,000; the verdict needs TLC's "No error" and a complete
  search depth below the guard) and `--kind`. Nine runs: `module`, `module-0332`,
  `module-generic`, `threads-module` (twice), `threads-module-kept`, `render`, `mixed` and
  `threads`, with allocation bounds of two to four module calls and one or two cells. All reach
  their fixed points with no error, at depths 19 to 35, 88 minutes in all on 4 cores; the same
  scopes with `FixCallPin` or the owning links off still find their counterexamples. The 6/2
  bounds of `module-0332`, `module-generic` and `render` did not finish in 30 minutes and stay
  depth-bounded only.
- `--simulate SECONDS --depth N --seed S`: TLC's random simulation with the guard off, pass
  runs by default. Measured on mutants: it finds all 17 mutants tried (F1, the loop and sort
  counterexamples among them) within 78 s. It is a sample, so it complements the exhaustive
  runs and does not replace them.
- Finding, fixed here: since 0346 (`FixSnapshotLinks`) `OwningLink` made every link outside a
  snapshot owning, so `drop-without-source-check`, `module-links-unowned` and
  `render-links-unowned` no longer found counterexamples and `render-handoff-pin-on-attach`
  did not test 0340 option (a). The `rest` shard's first full run since then timed out on it.
  The snapshot term now covers a global's adapter only; all four behave as intended again.
- The weekly job has two more shards, `exhaustive` and `simulate` (each pass run 15 minutes,
  40 states); the `rest` shard keeps the depth-bounded pass, mutant and finding runs.
- The README states per run whether it is depth-bounded (the Results table) or exhaustive (the
  new table).

Item 3 stays with 0338 step F and task 0363.
