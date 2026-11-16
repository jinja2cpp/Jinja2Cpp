---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/916
priority: medium
area: perf
depends: [0515]
touches: [src/node_arena.h, src/node_arena.cpp, src/template_env.cpp, src/template_env_impl.h, src/template_impl.h, include/jinja2cpp/template_env.h, src/chat_template.cpp, python/src/environment.cpp, bench/jinja2cpp_bench.cpp, test/template_env_test.cpp, docs/api-2.0.md]
---
# The rest of a Load's fixed cost: settings compare, sealing, template object

**Problem.** After 0515, `Load/plain_text` is 3,338 instructions and 3 allocations, about 0.72x of
inja's Load of the same case (`perf-track/2026-11-15-engines.md`, inja 186 ns). What is left is
spread over a few fixed costs of every Load, each a few percent:

- `GetLoadSettings` compares the whole `Settings` (~290 Ir) and takes a mutex (~60) on every
  template constructed with an environment. The compare cannot be skipped while
  `TemplateEnv::GetSettings()` hands out a mutable reference the host may edit at any time; skipping
  it needs a change of that contract (for example: edits through the reference take effect at the
  next `SetSettings` or the next mutable `GetSettings` call), which is a public-API decision.
- Sealing copies every node into an exactly sized buffer, relocates and reference-checks each one
  and destroys the originals (~450 Ir with the allocation). A tree that fits the arena's inline block
  and owns nothing could be sealed with fewer steps; the reference checks stay.
- `~TemplateImpl` (~280 Ir) and the template object's construction (~110 Ir plus the allocation).

**Decision needed.** The first item changes when edits through the mutable `GetSettings()` reference
take effect; it goes to Ruslan with the numbers before any code. The other two are internal.

**Acceptance.** Each step counted with `bench/count.py` against master: `Load/plain_text` down, no
row over the bar; the node reference checks of Seal stay (`JINJA2CPP_NODEREF_CHECKS`).

**Decision (Ruslan, 2026-11-16: option C).** Keep the mutable `GetSettings()` and its contract, and
stop paying for it when it is not used:

- `TemplateEnvImpl` remembers whether the mutable `GetSettings()` reference was ever handed out
  (`m_settingsLent`, set under `m_loadSettingsGuard`, never reset). Until then `GetLoadSettings` copies
  the cached `shared_ptr<const LoadSettings>` under the lock and compares nothing; afterwards it compares
  as before, so edits through the reference still reach the next template.
- `SetSettings` and the new `TemplateEnv::ModifySettings(const std::function<void(Settings&)>&)` publish
  a fresh snapshot under the lock (copy on write): a template made meanwhile gets the old or the new one,
  never a mix. `ModifySettings` runs its callback on a copy outside the lock; when it throws nothing
  changes. Configuration is otherwise as unsynchronised as before (docs/api-2.0.md, 3.10 rule 4).
- The library's own callers moved off the mutable reference: `ConfigureForChatTemplates`, the Python
  module (`ModifySettings`) and `bench/jinja2cpp_bench.cpp` (a copy given back with `SetSettings`). The
  benchmark change is what shows the gain: the base commit's benchmark still takes the mutable
  reference and compares.
- Note: on a non-const `TemplateEnv`, `env.GetSettings().x` picks the mutable overload, so a host that
  only reads that way also switches to the compare; `std::as_const(env).GetSettings()` does not.
  Whether to deprecate the mutable overload in 2.0 is a separate decision.

**Change (internal).**
- `~SealedArena` frees the buffer without the destroy walk when the tree owns nothing (`objects == 0`),
  `~NodeArena` skips `DestroyNodes` once Seal took the nodes, and `MoveNodes` has a loop without the
  block lookup for a tree in one block. The node reference checks are unchanged.
- `OutputSizeHint`'s cache-line padding is no longer zeroed (never read).

**Result** (GCC 13.3 Release, pinned name-hash key, `bench/count.py` against master 9541b58):

| Row | Before | After | Change |
|---|---:|---:|---:|
| `Load/plain_text` | 3,338 | 2,919 | -12.6% (settings -302, rest -117) |
| `Load/substitute` | 7,118 | 6,639 | -6.7% |
| `Load/for_range` | 12,931 | 12,407 | -4.1% |

Every other row is within -1.4% to +0.14%; allocations unchanged. `Retained` of `Load/chat_llama`
+16 B and `Load/hf_gpt_oss` -64 B with the same requested bytes: glibc's block sizes, not new memory.

**Left.** Seal still copies and relocates every node (about 25 Ir a node with its checks) and allocates
the sealed buffer; the template object's two allocations (impl, sealed buffer) and the mutex of
`GetLoadSettings` (~57 Ir) remain. A further cut would co-allocate the sealed buffer, a larger change.
