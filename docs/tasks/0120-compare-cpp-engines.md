---
status: open
priority: low
area: perf
depends: [0119]
touches: [bench/]
---
# Compare with other C++ template engines

**Problem.** Python Jinja2 is our only reference point. Embedders choosing a C++
engine compare against inja and minja (llama.cpp's Jinja subset), and we do not know
where we stand.

**Proposal.** An optional driver (off by default, fetched only when enabled) that renders
the cases each engine supports, reported next to Python in run.py.

**Done when.** bench/README.md has a table of the shared cases with all engines.
