---
status: in-progress
pr:
priority: low
area: robustness
depends: [0338, 0351]
touches: [tla/CellInductive.tla, tla/CellApalache.tla, tla/apalache.py, tla/CellProtocol.tla, tla/README.md]
shares: [.github/workflows/tla-model.yml]
---
# Prove the cell protocol's invariants inductively

**Problem.** Task 0338 step F planned inductive invariants for `tla/CellProtocol.tla`, checked
by Apalache: `IndInv /\ Next => IndInv'` for every state, reachable or not. Step F built the
harness (`tla/apalache.py`, `CellApalache.tla`, `CellInductive.tla`, README "Past the bounds")
and measured it. On the smallest module scope (6 adapters, 2 cells, 3 storages, 1 export) one
inductive step of the full `Next` ran out of memory at 14 GB, and `SnapshotStable` or
`NoCrossThreadWrite` alone took over 15 minutes against `IndInit`, which implies them. Finding
`IndInv` takes dozens of such rounds. Ruslan decided (2026-10-30) to ship the harness,
the well-formedness invariant and a cutoff argument on paper, and to defer the proof here: the
model is a design tool, and the implementation is checked by the periodic validation of a
frozen version (0364).

**What the proof would add.** For fixed constants, little: every scope is finite (README,
"Depth: every scope is finite"), so a completed run without the level bound (0351 item 1)
covers every behaviour too. The gain is speed if the inductive check gets cheap, and with
TLAPS, every size, which would replace the cutoff argument.

**What would make Apalache tractable**, in the order to try:
1. One inductive query per action, not for `Next`: `apalache.py inductive --next` already takes
   an operator; `CellInductive.tla` has `NextStart`, `NextUse`, `NextGC` and others as probes.
2. Flatten the indirect reads: `cl[ad[a].cell].stor` makes an if-then-else over the domain at
   every use. A derived variable per adapter (its cell's storage) or functions keyed by the
   pair would cut that.
3. GC: check a heap lemma ("in a quiet state the alive objects are exactly the reached ones";
   "nothing reached is dead") once, in a heap-only module, and use it as an assumption of the
   protocol steps instead of computing `Close` in each.
4. `snap` as a pair `<<Int, Int>>` with a flag, not a sequence that is empty or of length 2.
5. `wr` with one thread is constant: drop it from one-thread scopes.

**The cutoff gaps** (README, "Sizes: the cutoff argument"): the thread reduction needs
`MaxDepth` one higher for a call moved into another thread's render; the identity cutoff is
`MaxDepth + 2 + MaxEx` addresses, which no scope uses (they use 2; this PR's runs use 3). A
run with 6 addresses on a narrow module scope, a two-thread run at depth 2 and a TLAPS proof
of the thread reduction would close them.

**Done when** `IndInv` is shown inductive for at least the `module` scope (per action is
enough) and implies `Safety` (`apalache.py safety`), or the README states why the cutoff
argument and the exhaustive runs of 0351 are the final answer.

## Result (2026-11-26)

Closed by the second branch of the done-when: tla/README.md, "The inductive proof: why the
exhaustive runs are the answer", gives the reasons; the proof itself is task 0586.

- **The harness had rotted.** Since 0338 F, steps D, 0346 and 0463 added constants
  (`FixGlobalOuterCell`, `FixSnapshotLinks`, `FixAtomicLink`, `FixAtomicPin`), adapter fields
  (`snap`, `blk`), flags (`torn`, `glost`) and hosts (`GI`, `J`) that `CellApalache.tla`,
  `CellInductive.tla` and `apalache.py` never learned: the type check failed, and once fixed,
  `IndInit` had no state at all (Gen sized each registry for 3 hosts of 5), so every inductive
  query would have passed. Fixed; `apalache.py` reads `Hosts` from the spec, `Safety` lists
  `NoTornStep`, `NestedGlobalKept` and `CallScopedBounded`, and the weekly `tla-model` workflow
  gains an `apalache` job (type check, `Init => IndInv`, `IndInit` has a state) so it cannot rot
  silently again.
- **Lever 1 measured, and a better one found.** One inductive query per action and per
  conjunct of `IndInv` (`apalache.py sweep`, `--hyp`): a part takes 20 s to a few minutes, the
  conjunction of the same parts in one query over 15 minutes. 65 of the first 70 queries hold;
  4 reads through an adapter time out at 10 minutes; `Read => NoForeignCell'` gives the first
  counterexample to induction (an adapter attached to a cell of other data, in a render
  registry of a scope without renders).
- **Cutoff gaps.** Two threads at `MaxDepth=2` reach their fixed point with no error
  (`threads-exhaustive` 219,938 states, `threads-module-exhaustive` 19,304,429). The identity
  cutoff is exact for the exhaustive bounds: `MaxAd / 2` calls name at most that many addresses,
  and the three-address run of `module-0332-exhaustive` (13,018,093 states) covers its bounds. Left on paper: the thread reduction (TLAPS, task 0586).
