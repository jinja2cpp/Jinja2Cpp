---
status: done
priority: low
area: robustness
depends: [0478]
touches: [src/recursion_guard.h, src/internal_value.h, src/internal_value.cpp, src/template_dict.h, src/template_dict.cpp, src/value_methods.cpp]
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

**Done.** Each counted object records whether a budget was active when it was made, and only those give bytes back:
- `ValueObject` packs `m_isRange` and `m_isBorrowed` into one byte of bits and adds a Charged bit, set by its
  constructors while a budget is active (never copied; `sizeof` stays 16, asserted). Its destructor leaves
  `t_freedCharged` (recursion_guard.h) for `ValueObject::operator delete`, which has no object left to ask and
  credits its size only if the mark lies in the freed block. `StringObject`, `BigIntObject` and the list `Storage`
  release only if charged; `CountedItems` and `TemplateDict` carry their own flag, set when they are made, that a
  move takes with the entries. `list.sort()` asks the list's accessor (`IListAccessor::ItemsCharged`).
- An object not charged that a counted render grows keeps the growth charged and is never credited back: charging
  too much is safe, crediting too much is a bypass. An object charged by one render and freed in another still
  credits the render that frees it; that is bounded by what the first render made.
- Test: `LimitsCachesTest.FreeingUncountedObjectsGivesNoBudget` (no template reaches an uncounted object that a render
  frees repeatedly any more, so it frees strings, lists and dicts made without a budget and under
  `UncountedMemoryScope` inside a budgeted scope).
