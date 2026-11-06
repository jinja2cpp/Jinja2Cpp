---
status: open
priority: low
area: ci
depends: [0359]
touches: [.github/workflows/benchmark.yml, scripts/preflight.sh, bench/jinja2cpp_bench.cpp]
shares: [bench/README.md]
---
# Detect the base bench driver's features by a capability marker

**Problem.** The instruction gate runs the base (master) driver on the head's `bench/cases`, and
falls back to the base's own cases when the base driver lacks a feature a new case needs. The
check is a chain of `grep`s for settings keys (`"transformers"`, `"prompt_specials"`) in the base
driver's source, duplicated in `.github/workflows/benchmark.yml` and `scripts/preflight.sh`.
Each new feature case needs a new line in both (found while adding `prompt_budget`, 0359).

**Proposal.** Give the driver a `--capabilities` flag (or a version number) that lists the
settings keys it understands, and give each case's `settings.json` the keys it needs; the gate
runs a case on the base only when the base lists them, per case rather than all or nothing.
Share the logic in one script used by both the workflow and preflight.

**Acceptance.** Adding a case with a new settings key needs no gate edit; existing cases are still
compared on both sides.
