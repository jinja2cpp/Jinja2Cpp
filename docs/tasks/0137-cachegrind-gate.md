---
status: open
priority: high
area: perf
touches: [bench/count.py, bench/trend.py, .github/workflows/benchmark.yml, bench/README.md]
---
# Cache misses are not measured

**Problem.** The 0117/0118 design (docs/design/perf-design-overview.md) makes claims about
data and instruction cache misses (Load D1 misses -40..-50%, `many_tags` render misses
13.6k → 3-4k). `bench/count.py` records instructions, allocations and memory only, and
`perf` is not available on the runners or in the cloud container.

**Proposal.** `count.py --cache-sim` runs callgrind with `--cache-sim=yes` and adds D1mr,
D1mw, DLmr and I1mr columns per case, Load and Render. The trend job records them on
`bench-data` (README table and a chart). The PR gate shows them but does not fail on them
(cachegrind's cache model is idealised: read relative changes).

**Done when.** Columns appear in `count.py` output, the trend and the PR summary;
bench/README.md says how to read them.
