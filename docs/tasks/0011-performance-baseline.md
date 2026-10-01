---
status: open
priority: low
area: perf
---
# Re-enable performance tests and track a baseline

**Problem.** Performance is a stated goal, but the perf tests in `test/` are disabled and
nothing measures regressions.

**Proposal.** Turn them into a Google Benchmark target, run on a schedule on a fixed runner
type, store results as artifacts and compare against the previous run.

**Done when.** A scheduled job publishes benchmark numbers and flags regressions above a threshold.
