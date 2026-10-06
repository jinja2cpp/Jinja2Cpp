---
status: done
priority: medium
area: perf
touches: [src/template_impl.h]
---
# Every render writes a cache line that all threads read

**Problem.** `TemplateImpl::m_outputSizeHint` (src/template_impl.h, `mutable std::atomic<size_t>`)
is stored on every render, next to read-only fields every render reads (the renderer root).
When several threads render one template, each store invalidates that line in the other
cores (false sharing).

**Proposal.** Store only when the new hint differs from the old by more than a margin
(e.g. 25%), and put the atomic on its own 64-byte line (`alignas(64)` member or a padded
holder). Measure with `jinja2cpp_bench --threads 4` on `plain_text`, `substitute` and
`inheritance`.

**Done when.** No store on a steady-state render; MT throughput measured before and after.
Expected +5..+15% on small templates at 4 threads (low-medium confidence).

**Result (PR #421).** The hint is padded onto cache lines of its own and stored only when
the output outgrows it or needs less than half of it, so a render of a steady size stores
nothing (an output that grows now and then stores each new maximum once).
`MT/Render` renders per second, median of 3 runs on a 4-core cloud container, before → after
this change (both with 0139 a+b): `plain_text` 4 threads 5.5M → 43.6M, 2 threads 5.8M →
24.0M; `substitute` 4 threads 7.1M → 18.1M; `inheritance` within noise.
