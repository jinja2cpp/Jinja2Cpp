---
status: open
priority: medium
area: ci
depends: []
touches: [.github/workflows/fuzz.yml]
shares: [.github/actions/setup-ccache/action.yml]
---
# The pull-request `fuzz render` job runs close to its 45-minute timeout

**Problem.** Found on PR #989 (task 0402). A pull request's `fuzz render` job has 45 minutes
(`timeout-minutes` in `.github/workflows/fuzz.yml`). Inside that time it builds the ASan+UBSan
target from scratch, then replays a corpus of about 21,600 inputs. The job was cancelled at that
limit twice on #989, after 35 minutes of replay each time, with no crash and no sanitizer
report. A re-run of the same commit (407f35f) passed, with a replay of 22m22s. Measured on
2026-11-22:

| Run | Build | Replay | Result |
|---|---|---|---|
| #989 407f35f, attempt 1 | 9m20s | over 35m23s | cancelled |
| #989 407f35f, attempt 2 | 5m55s | 22m22s | success |
| #989 2bb3783 | 9m00s | over 35m42s | cancelled |
| #993 2f2ed96 | 5m59s | 22m36s | success |
| #987 train 3fa1330 | 8m39s | 30m33s | success |

All five runs restored the same corpus (`fuzz-corpus-render-45009102035`), and all built with a
cold compiler cache: ccache reported `Hits: 0 / 67` each time. Pull requests only restore the
cache that non-PR runs save (`setup-ccache`), and none was found for
`fuzz-clang-18-render-bigint`. Each timed-out input also costs its 25 s `-timeout`, and runs saw 3
to 6 of them. A slow runner therefore crosses 45 minutes with no change in the code, and the
failure lands on whichever PR is unlucky.

**Proposal.** Raise the pull-request timeout of the `render` target (or of all fuzz jobs) to about
60 minutes. Find out why the nightly run's saved ccache is not found by pull requests (key,
eviction, or a nightly that does not reach the save), so that the build stops being cold.
Optionally, have `fuzz/replay_corpus.py` print per-batch progress, so that a slow batch shows in
the log.

**Done when** the `fuzz render` PR job has at least 15 minutes of headroom over the slowest
replay measured above, its build gets ccache hits on a pull request, and the change is recorded
in the workflow's comments.
