---
status: open
pr:
priority: medium
area: perf
depends: [0515]
touches: [src/node_arena.h, src/node_arena.cpp, src/template_env.cpp, src/template_env_impl.h, src/template_impl.h]
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
