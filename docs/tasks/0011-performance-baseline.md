---
status: in-progress
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

**Next.** Timings on shared runners vary by 5-15%, too much to flag a 10% regression.
Gate on something stable instead: instruction counts (`valgrind --tool=cachegrind` or
`perf stat -e instructions` on a runner that allows it) per benchmark, compared with the
last master run stored as an artifact or on a `gh-pages`-style data branch, plus a
scheduled wall-clock run that only reports trends.
