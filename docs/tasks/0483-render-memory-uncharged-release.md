---
status: open
priority: low
area: robustness
depends: [0478]
touches: [src/recursion_guard.h, src/internal_value.h, src/internal_value.cpp]
shares: [test/limits_test.cpp, docs/security.md]
---
# Give the render memory budget back only what it was charged

**Problem.** `ReleaseRenderMemory` credits the running render with the bytes of any string, list, dict or object
freed while it runs, up to the whole budget, whether or not that render (or any render) was charged for them: an
object made before the render, by another render, or under `UncountedMemoryScope` (a loaded template, the globals
snapshot, 0478) gives budget it never took. The security review of #863 found one path a template drives: an
`EveryUse` include without a cache freed its dropped reload inside the render and kept about 100 MB under a 1 MiB
budget (fixed there by freeing it uncounted, `LimitsCachesTest.DroppedReloadGivesNoBudget`). Any other place
where a render frees uncounted objects it can repeat is the same bypass.

**Proposal.** Record on each counted object whether it was charged (a bit in `ValueObject`'s padding is taken: pack
`m_isRange` and `m_isBorrowed` into flags, or charge through a per-render generation) and release only charged
bytes; or make every place that frees cache-owned objects inside a render free them under `UncountedMemoryScope`,
with a contract check. Measure with `bench/count.py`.

**Done when** freeing objects the render was not charged for never raises `memoryLeft`: a `limits_test` row that
frees host-made or cache-made values in a loop while keeping its own still fails with `maxRenderMemory`.
