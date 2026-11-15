---
status: open
priority: medium
area: robustness
depends: [0459, 0478]
touches: [src/render_context.h, src/template_module.cpp, src/internal_value.cpp]
shares: [docs/security.md]
---
# The render memory budget does not count scope maps, imported modules or closure copies

**Problem.** Found by the security review of #891 (0231), present on master before it. `maxRenderMemory` (0459)
charges through class allocation functions (`ValueObject::operator new`, strings, list slots and dict entries),
and 0478 left `std::make_shared` objects uncounted ("Next"). Scope maps (`InternalValueMap`, robin_hood before
0231 and unordered_dense after) are not charged either, nor the module an `{% import %}` builds. Under
`UntrustedSettings`, 100,000 `{% import "x" as m with context %}` in a loop peaked at 524 MB on master and 698 MB
with 0231's segmented maps; 50,000 kept closure copies at 136 MB and 165 MB. Both are past the 256 MiB default
budget, so a template can exceed it many times over without the render failing.

**Proposal.** Charge what a scope and a module hold when it is made: either an allocator for `StableMap` that
charges the render's budget (the map type is one alias in src/hash_map.h), or a fixed charge per map and per
module taken where they are created (EnterScope, the module builder, closure capture), credited back on
release under 0483's rule (release only what was charged). Measure the per-node cost with bench/count.py.

**Done when.** Both loops above fail with the memory-limit error under `UntrustedSettings`, a test covers each,
and the bench rows stay within the perf bar.
