---
status: open
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
