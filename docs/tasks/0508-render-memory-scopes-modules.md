---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/932
priority: medium
area: robustness
depends: [0459, 0478]
touches: [src/recursion_guard.h, src/recursion_guard.cpp, src/hash_map.h, src/internal_value.h, src/internal_value.cpp, src/render_context.h, src/ankerl/unordered_dense.h, src/statements.h, src/statements.cpp, test/limits_test.cpp, test/allocation_failure_test.cpp]
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

**Outcome (in progress).** `InternalValueMap` (every scope, the names a with-context import or module sees, the
copy a kept closure holds) allocates through `CountedAllocator` (src/recursion_guard.h): each block is charged as
an object is (`ChargeObjectMemory`: past the budget the render fails at its next step, nothing throws from an
allocator) and records its charge in a 16-byte header, so freeing it gives back exactly what it took and a map made
before the render, without a budget or under `UncountedMemoryScope` gives nothing (0483). `ImportedModule` (about
1.6 KB with its context) allocates through the same block, and so do a closure's `ClosureCapture` and its copied
names (`allocate_shared`). No per-map fixed charge, so the per-node path pays one compare per map allocation.
The allocator propagates as `std::allocator` does, and the vendored ankerl keeps a map's list of segments in its
first segment for any stateless allocator (it did for `std::allocator` only, 0506), so a scope's first insert
still allocates twice. Instruction counts (preflight --perf): every row within +0.22% (Render/hf_gpt_oss), no new
allocations, 16 bytes more per map block; GCC stopped inlining AsString's narrow copy in internal_value.cpp
(+0.8% on Render/chat_llama), which a flattened helper restores, and `TakeTopScope` is forced inline (plain_text
rows -1%).

Measured in Debug, `UntrustedSettings`: 100,000 `{% import "x" as m with context %}` now fail with
`maxRenderMemory` at a 324 MB peak (rendered at 525 MB before); 10,000 of them took 2-5 MB of the budget before and
30-60 MB now. 50,000 kept closures took 5-10 MB of the budget before and 60-80 MB now; their 125 MB peak is below the
256 MiB default, so the test bounds them with 16 MiB. Tests (`test/limits_test.cpp`):
`KeptModulesCountAgainstTheBudget` and `KeptClosureCopiesCountAgainstTheBudget` (both fail on master, narrow and
wide), with dropped scopes and the shared without-context module rendering in 1 MiB.

Still uncounted (filed as 0530): what a macro's `std::function` holds apart from its `Callable` (about 124 bytes a macro) and the
vectors inside a module's `RenderContext`; the budget remains an estimate, as docs/security.md says.
