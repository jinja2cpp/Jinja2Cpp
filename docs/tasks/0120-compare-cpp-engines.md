---
status: done
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

**Done** in the PR that adds this line: `-DJINJA2CPP_BENCH_WITH_OTHER_ENGINES=ON` builds
`bench/engines_bench.cpp` with inja v3.5.0 and minja (ochafik/minja), fetched by
`FetchContent` at pinned commits; `run.py --engines` adds a column per engine for the
cases whose output matches Python Jinja2. Results and what each engine cannot run:
bench/README.md "Other C++ engines". Jinja2C++ renders every shared case fastest
(1.1-31x); inja's faster `Load` on small templates is filed as 0131.
