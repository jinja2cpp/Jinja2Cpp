---
status: open
pr:
priority: low
area: perf
depends: []
touches: [bench/trend.py, .github/workflows/benchmark.yml]
---
# The drift report cannot see creep slower than 3% per 10 merges

**Problem.** Found by the 2026-11-16 perf scout (`/mnt/project-files/perf-track/2026-11-16-scout.md`).
The `trend` job runs `trend.py drift --window 10 --threshold 0.03` (.github/workflows/benchmark.yml),
which compares the newest record with the record 10 records earlier. Master now takes about 40
engine merges a week, so a window is one or two days. A row that grows by 0.3-0.9% at a time, with
each step a named exception, never crosses 3% within one window. `Module/keep` is +4.26% since
2026-10-28 over 11 steps (task 0519), and the job reported "No new drift above 3% over the last 10
records" on every run.

Until now the weekly scout was the only thing that caught this, by hand.

**Proposal.** Give `trend.py drift` a time anchor next to the record window:
- `--since-days N` compares the newest record with the newest record at least N days older;
- the job runs it with N = 7 and N = 28 beside the current window.

Keep the rule that a case is reported only when it newly crosses the threshold. A case that has
already been accepted then does not repeat on every merge. Use 3% for the 7-day anchor and 5% for
the 28-day anchor. List the steps of 0.2% or more inside the window in the report, as the scout's
tables do, so that the issue names the merges.

**Done when.** Replaying `history.jsonl` from `bench-data` at 37c378c reports `Module/keep` for the
28-day anchor and nothing new for the 10-record window, and the job's issue text shows the steps.
