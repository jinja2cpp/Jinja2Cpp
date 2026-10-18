---
status: open
priority: medium
area: agents
touches: [scripts/task_index.py, scripts/task_batches.py]
---
# Two task files can share a number

**Problem.** Task numbers are picked by hand from master and the open PRs, and
parallel threads race for the next free one. Nothing checks the result. The 2.0
release readiness branch filed 0212-0218 while master already had 0212 and 0213 and
open PRs held 0214-0216; `scripts/task_index.py --check` accepted the merged tree and
emitted two index rows for each duplicated number, and `scripts/task_batches.py`
reported conflicts as bare number pairs that no longer name one file.

**Proposal.** Make `task_index.py --check` report every number used by more than one
file in `docs/tasks/`, and fail with `--strict`; have `task_batches.py` refuse to plan
with duplicates. The format check then catches a collision on a train branch before
merge.

**Done when.** A tree with two `NNNN-*.md` files for one number makes
`task_index.py --check --strict` exit 1 and name both files.
