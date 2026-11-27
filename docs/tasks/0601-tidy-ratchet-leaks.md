---
status: in-progress
priority: high
area: ci
depends: [0600]
touches: [.github/workflows/clang-tidy.yml, scripts/preflight.sh]
---
# clang-tidy ratchet: hits on unchanged lines pass the pull-request job, and a red weekly run is silent

**Problem.** The pull-request job runs `clang-tidy-diff.py` and reports only diagnostics on changed lines.
That keeps the job fast and keeps unrelated old hits out of a PR, but many checks anchor their diagnostic
on a line the PR did not touch:

- `readability-function-cognitive-complexity` reports the function's first line; adding a branch in the
  body pushes the score past 25 without a reported hit (39 on master, 0600).
- `misc-include-cleaner` reports the `#include` line; removing the last use of a header leaves an unused
  include on an unchanged line (60 on master).
- A type change moves hits elsewhere: making `Token` trivially copyable turned 35 unchanged
  `std::move(tok)` calls into `performance-move-const-arg` errors.
- `modernize-use-nodiscard`, `pro-type-member-init`, `virtual-class-destructor` and
  `special-member-functions` report the declaration, not the body or member that changed.

`scripts/preflight.sh` has the same blind spot, so the agents' local check cannot catch it either. The
only full check is the weekly job, and its failure reaches no one: it has failed 8 weeks out of 8
(0600). So the ratchet that 0054/0081 set up holds per line, not per tree.

**Proposal.** Once 0600 makes master clean, any hit in a file a PR touches is that PR's:

1. Pull requests: run clang-tidy on every changed translation unit and changed header (whole file, not
   changed lines), keeping `-quiet` and the header filter. Annotate all its hits; fail on any. A header
   change also runs on one translation unit that includes it, as today. Measure the job time on a
   typical PR before and after; if it grows past about 15 minutes, keep changed-line mode for `test/`
   and run whole-file mode for `src/` and `include/` only.
2. `scripts/preflight.sh`: the same whole-file mode, so agents see it before pushing.
3. Make the weekly red visible: on a scheduled failure, open or update one GitHub issue (label
   `clang-tidy`) with the per-check table, and close it on the next green run. The hourly stall-sweep or
   the backlog routine reads open issues with that label.
4. The merge steward's train runs `workflow_dispatch` of `clang-tidy.yml` on the train branch (whole
   tree) before merging, or the train job adds the whole-tree step (docs/steward-checklist.md).

**Done when.** A PR that adds one branch to a function at score 25, or removes the last use of an
included header, fails the clang-tidy job; preflight reports the same; a red scheduled run opens an
issue.

**Outcome.** The PR job and `scripts/preflight.sh` both run `scripts/tidy_changed.sh`: clang-tidy on every
changed `.cpp`/`.h` as a whole file (a translation unit with its own flags, a header with the flags of the
nearest file in the database, as the weekly job does). A red weekly run opens or comments on one issue labelled
`clang-tidy` and a green one closes it; the steward's train dispatches `clang-tidy.yml` on the train branch
(`docs/steward-checklist.md`, step 7). `bindings.yml` keeps its changed-lines job: follow-up 0606.
