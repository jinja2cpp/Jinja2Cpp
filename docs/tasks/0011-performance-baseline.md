---
status: done
priority: high
area: perf
touches: [test/perf_test.cpp, bench/, .github/workflows/benchmark.yml]
shares: [CMakeLists.txt, thirdparty/internal_deps.cmake]
---
# Re-enable performance tests and track a baseline

**Problem.** Performance is a stated goal, but the perf tests in `test/` are disabled and
nothing measures regressions.

**Proposal.** Turn them into a Google Benchmark target, run on a schedule on a fixed runner
type, store results as artifacts and compare against the previous run.

**Done when.** A scheduled job publishes benchmark numbers and flags regressions above a threshold.

**Progress.** Step 1 (this task's first PR): `test/perf_test.cpp` is replaced by the
`jinja2cpp_bench` Google Benchmark target in `bench/` (`-DJINJA2CPP_BUILD_BENCHMARKS=ON`),
with 14 workloads shared with a Python Jinja2 driver, so each number has a reference.
`bench/run.py` checks both engines render identical text, prints a comparison table and
compares against a baseline file. `.github/workflows/benchmark.yml` builds and runs it on
PRs touching the engine and publishes the table to the job summary; it does not gate.

First baseline (Release, GCC 13, 4-core cloud container): loading is 5-22x faster than
Python Jinja2, rendering ranges from 9x faster (tiny templates) to 3-5x slower
(`Render/mitsuhiko_table`, `Render/expressions`). Findings filed as 0086, 0087, 0088.

Step 2: `bench/count.py` counts instructions per benchmark iteration under callgrind
(repeatable to about 0.05%, the whole suite in about 10 s). The `instructions` job builds
the base commit and the PR head on one runner and fails a PR that makes any benchmark
more than 3% more expensive. The wall-clock job also runs nightly on master, keeping 90
days of results as artifacts.

**Trend and memory.** Allocations per iteration are counted by a replaced `operator new`
in the driver and reported by `count.py` next to the instructions (#368, which also added
gperftools profiles and threaded renders). The `trend` job keeps both counts per master
commit on the `bench-data` branch and draws them with `bench/trend.py` (README table plus
one SVG chart per benchmark).
