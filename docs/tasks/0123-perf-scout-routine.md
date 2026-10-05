---
status: open
priority: medium
area: perf
touches: [.github/workflows/benchmark.yml, bench/trend.py]
---
# A recurring perf scout and a drift alert on the trend

**Problem.** The PR gate (0011) compares a PR with its base, so it misses slow drift
over several PRs that are each under 3%, and nobody re-profiles master unless asked.
Ruslan asked for recurring asynchronous perf work.

**Proposal.**
1. Drift alert: the `trend` job compares the new point with the median of the last
   N points and, when any case moves by more than 3%, comments on the merged PR (or
   opens an issue) with the table.
2. A weekly scout routine (a scheduled project session): build master, run
   `bench/run.py` and `count.py`, profile the three cases closest to Python, compare
   with the previous week's top frames, and file or update tasks under `docs/tasks/`.

**Done when.** The drift alert fires on a deliberately slowed test branch, and the
routine has produced its first weekly report.
