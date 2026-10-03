---
status: open
priority: low
area: process
depends: [0003, 0094]
touches: [fuzz/differential.py]
shares: [test/parity/cases/fuzz.py, test/parity/divergences/fuzz.txt]
---
# Triage the nightly differential report into parity cases

**Problem.** The nightly fuzz workflow (0003) ends with `fuzz/differential.py`, which
renders the libFuzzer render corpus with Jinja2C++ and Python Jinja2 and uploads a
Markdown report of the differences. Nobody reads it: the first run's findings were
minimised and pinned by hand (0094), and new ones will pile up in workflow artifacts that
expire after 90 days. Most entries are variants of known divergences, so the report is
long and the new part is hard to see.

**Proposal.**
- `differential.py --known`: also skip inputs whose difference matches a case already
  listed in `test/parity/divergences/*.txt` (same kind and same feature), so the report
  shows only new kinds of divergence. Group the rest by kind (output, accepts, rejects)
  and by the first differing filter, test or statement.
- A recurring Claude routine (scheduled after the nightly run) that downloads the newest
  report, minimises each new divergence by hand-checking it against Python, adds it as a
  case under `test/parity/cases/` with an allow-list line owned by a task, and opens one
  PR per batch through the merge steward. Crashes stay with the fuzz job, which fails on
  them.

**Done when.** The nightly report lists only divergences the corpus does not know, and
new ones reach `test/parity/` within a day without a person starting the work.
