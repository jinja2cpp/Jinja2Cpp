---
status: open
priority: low
area: perf
depends: [0093]
touches: [src/value_visitors.h, src/serialize_filters.cpp]
shares: [src/internal_value.h, src/internal_value.cpp, src/recursion_guard.h]
---
# Render cost of the deep-value stack checks and trashcan release (0093)

**Problem.** 0093 (#529, merged in train 13, #535) made deeply nested values safe: the
walks over values call `CheckStack()` once per level, and the last release of a
`ValueObject` goes through a hand-written atomic count and the exported
`detail::DestroyValueObject`. Train 13's instruction counts (febaac3 against f11b3d2,
job 117350609658) show Load flat and these Render changes:

| Case | Change |
|---|---|
| Render/chat_qwen | +1.34% |
| Render/chat_llama | +1.30% |
| Render/strings | +1.05% |
| Render/filters | +0.82% |
| Render/chat_mistral | +0.55% |
| Render/expressions | -2.78% |
| Render/macros | -0.75% |
| Render/for_filter_if | -0.61% |

Every case that got slower renders through `tojson` or the value printer. The cause has
not been profiled. The candidates are the per-level `CheckStack()` in
`ValueRendererBase`, `PrettyPrinter` and the `tojson` writer (`src/value_visitors.h`,
`src/serialize_filters.cpp`), and the release path in `src/internal_value.h`
(`fetch_sub` plus the out-of-line `DestroyValueObject` call).

**Approach.**
1. Attribute the cost with `bench/count.py` (per-function counts) on chat_qwen and strings.
2. If `CheckStack()` dominates, check the stack only every N levels, or only once the
   walk is deeper than a threshold, such as 64 levels. Either way the walk must still end
   in `RecursionLimitExceeded` at 100000 levels; `RecursionLimitsTest.DeepValues` covers
   this, and its walks run on a 1 MiB thread.
3. If the release dominates, keep the `fetch_sub` inline and check that the common case
   (a flat object with no nested `ValueObject`) does not pay for the pending list.

**Done when.** Render/chat_* are back within +0.3% of f11b3d2, the other cases do not
regress, and `RecursionLimitsTest.DeepValues` passes normally and under ASan and TSan.
