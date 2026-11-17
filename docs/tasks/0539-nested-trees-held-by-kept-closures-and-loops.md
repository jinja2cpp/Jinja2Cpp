---
status: open
priority: low
area: robustness
depends: [0521]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, src/statements.cpp]
shares: [test/limits_test.cpp]
---
# Kept closures and a dropped recursive loop still hold nested trees after 0521

**Problem.** Found by the verifier of 0521. Since 0521 a render nested in another one that shares its
globals gives its tree back once nothing names it, but two holders outlive what names it:
- A closure that reads names, or a stored `self`, keeps its copy of the names in the collector's
  `KeptObjects` (0491), which hold the tree until that list prunes at 64 entries. The ledger's
  over-budget prune (`KeepNewTree`) gives back only expired trees. A 32 KB template, a 1 MiB budget,
  and a macro that reads a name, stored in `g` and popped the next iteration: the loop fails with
  `maxRenderMemory` after about 30 iterations, although one value is live. Master fails the same.
- A recursive `loop` stored and then dropped pins its tree until the keeper ends:
  `{% if g %}{% set _ = g.pop() %}{% endif %}{% for x in [1] recursive %}{% set _ = g.append(loop) %}{% endfor %}`
  rendered 20000 times with no budget keeps 20000 trees (about 160 MB). With a 1 MiB budget the
  render fails at about 325 trees. Master is no better.
Both are memory-safe. A stored `self` that is let go of is 0529's.

**Proposal.** On the over-budget path of `KeepNewTree`, prune `KeptObjects` of the closures nothing
holds before failing. For the loop, find the cycle that keeps it (its captured names or its
`recursiveOwner` reached from the loop state) and break it as 0529 does for `self`.

**Done when.** Both loops above with no budget keep at most a few trees (`KeptTreeCount`), and with a
1 MiB budget they render; `LimitsEnvTest` rows pin them, ASan clean.
