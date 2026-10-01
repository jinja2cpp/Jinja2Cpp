---
status: open
priority: medium
area: ci
---
# Coverage as a gate, not a number

**Problem.** The coverage job (PR #291) reports library line coverage (about 74% in a
local Debug run) but nothing acts on it, so it can drift down unnoticed.

**Proposal.** Publish per-PR coverage of changed lines (diff coverage) and fail when new
code in `src/` is under a threshold, rather than gating on the global percentage.
Uncovered areas list feeds 0001/0003.

**Done when.** A PR that adds untested code in `src/` gets a failing or flagged check.
