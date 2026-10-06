---
status: open
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
