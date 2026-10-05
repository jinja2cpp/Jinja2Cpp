---
status: done
priority: low
area: perf
touches: [bench/jinja2cpp_bench.cpp, bench/count.py, bench/trend.py]
---
# Track memory per loaded template and per render

**Problem.** The trend tracks instructions and allocation counts, not how much memory
a loaded template keeps or a render peaks at. A cache of thousands of templates (a
server with per-tenant templates) cares about the first.

**Proposal.** The counting allocator already sees every allocation: record live bytes
after Load and peak bytes during Render, print them with the counts and add them to the
trend charts.

**Done when.** `bench/count.py` reports both numbers and the trend shows them.

**Done** in the PR that adds this line: `jinja2cpp_bench --count` prints
`memory retained <n> peak <n>` from one extra iteration outside the counted region,
`count.py` reports them as `Retained`/`Peak` (with the change against a baseline) and
writes them to its JSON, and the trend keeps them per commit and draws a third chart panel
(Load: retained, Render: peak). The bookkeeping adds one flag check to operator new and
delete, +0.0..+1.1% instructions once. First readings and what they found: bench/README.md
"Memory"; template footprint filed as 0130.
