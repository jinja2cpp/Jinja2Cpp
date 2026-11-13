---
status: done
priority: low
area: robustness
depends: [0478]
touches: [src/recursion_guard.h, src/internal_value.h, src/internal_value.cpp, src/template_dict.h, src/template_dict.cpp, src/value_methods.cpp]
shares: [test/limits_test.cpp, docs/security.md]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/882
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

**Done.** Strings, lists and dicts give the budget back only what a render counted when it made them, with no byte
or allocation growth (the first design read the render-limits TLS in every `ValueObject` constructor and in its
destructor and `operator delete`, and cost up to +1.09% instructions):
- `ValueObject` packs `m_isRange` and `m_isBorrowed` into one byte of bits and adds a Charged bit, default 0, read by
  no constructor and never copied (`sizeof` stays 16, asserted). The makers set it through the protected
  `MarkCharged()` in the branch that charges: `StringObject::Make`, `BigIntObject::Make` and the list/dict
  `BySharedMutable::Storage` constructor, which charge only when `RenderMemoryCounted()`. Their destructors release
  only if `IsCharged()`. `Storage` has its own `operator new/delete`, since its `Bytes()` already counts the object.
  `~ValueObject` and `ValueObject::operator delete` are as before.
- `TemplateDict` keeps its counted flag in the low bit of the `KeyTable` pointer (`KeyTableHolder`), so it stays the
  size it was. The flag is set where the dict charges while it holds no entries (a new dict, a copy, a first entry),
  so that every entry it holds was counted; a dict that got entries while uncounted stays uncounted and is never
  credited (conservative). Moves take it with the entries. `Erase`, `Clear`, the destructor and move assignment credit
  only when it is set; an entry added with no budget active clears it.
- An object not charged that a counted render grows keeps the growth charged and is never credited back: charging
  too much is safe, crediting too much is a bypass. An object charged by one render and freed in another still
  credits the render that frees it; that is bounded by what the first render made.
- Not tracked: the other objects (loop state, callables, adapters) and the items of a generated list
  (`CountedItems`) are credited by their size when freed, as before. One made uncounted that a counted render frees
  credits its own size once; no template repeats that, as the caches free under `UncountedMemoryScope`, and a
  generated list is made and freed by renders.
- `list.sort()` asks the list's accessor (`IListAccessor::ItemsCharged`).
- Test: `LimitsCachesTest.FreeingUncountedObjectsGivesNoBudget` (no template reaches an uncounted object that a render
  frees repeatedly any more, so it frees strings, lists and dicts made without a budget and under
  `UncountedMemoryScope` inside a budgeted scope).
