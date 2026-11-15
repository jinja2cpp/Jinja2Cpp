---
status: in-progress
priority: high
area: performance
depends: [0231]
touches: [.github/workflows/benchmark.yml]
---
# Instruction counts vary between runs since the keyed name hash

**Problem.** Since #891 (0231), `detail::HashName` xors in a key drawn once per process (random_device,
clock, address). The probe lengths of the scope maps, and so the instruction counts, now depend on the key.
Four "instruction counts vs base" jobs on 2026-11-15 built the same base commit 01fbec9 and counted
Render/config_file at 1,278,942, 1,280,011, 1,281,595 and 1,298,517 Ir, a 1.5% spread. Render/many_tags and
Render/chat_mistral wandered by about 0.2%. A PR whose own diff did not change (#895, 0835841 and then
c995538 after a master merge) went from +0.46% to +2.06% on config_file. The perf bar is 0.5% per row, so the
gate could not tell a regression from a key.

**Change.** `src/hash_map.cpp` already has a build hook, `JINJA2CPP_NAME_HASH_KEY`, which fixes the key. The
head, base and trend builds in `.github/workflows/benchmark.yml` now define it with the same value. A base
that predates the hook ignores the define, and such a base predates the keyed hash, so its counts are already
deterministic. The released library still draws its key per process.

**Limits.** One fixed key is one sample of the probe-length distribution. A change can still shift which
names collide under that key, so a row that moves on a hash or map change should be checked under a second key
(rebuild with another `JINJA2CPP_NAME_HASH_KEY`) before it counts as real. The bench-data trend series
steps once, at the commit that pins the key.

**Done when.** The count job on this PR, whose src is identical in head and base, reports 0.00% on every
row, and two reruns give the same counts.
