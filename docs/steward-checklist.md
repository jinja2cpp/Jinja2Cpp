# Merge steward checklist

The merge steward is a standing thread. It rotates weekly (see CLAUDE.md, "Merge steward"), so
everything it has learned lives here, not in its context. A fresh steward reads this file first;
add a trap here in the PR that teaches it. Origin: the steward's own list from trains #968-#981
and the earlier trains of 2026-10, collected for the 2026-11-22 workflow review.

## Gates before a PR merges

- Green on the exact head sha, plus the owner's FINAL message. A relay from another thread is
  information, not approval.
- A Codex result with no unanswered P0/P1 (CLAUDE.md, "External review").
- No red CodeQL (neutral is fine) and no open review thread from Ruslan. Hold when the owner or a
  decision card says wait.
- Count checks with `pull_request_read get_check_runs` and `perPage` 100 (53-64 checks). Success,
  neutral (CodeQL) and skipped are fine.
- A single non-overlapping PR on current master may merge directly; its index update then goes in
  a small docs PR. Three or more finals go in one train, one CI run.

## Perf bar (any PR touching `src/` hot paths)

- Run the "instruction counts vs base" job through `perf-track/countjob.sh <job_id>` from the
  project files. Check that the head sha in its first line is the PR head and the base is the
  actual current master. Overlapping PRs need a fresh count against current master.
- Bar: +0.5% per row, +3% on the instruction gate, no allocation increase. A row at 0.5-1% is a
  named exception in the merge message (row, percent, base sha). Over 1% or any allocation
  increase: the owner fixes it, or ask Ruslan with a decision card. The "(+n)" markers on byte
  columns are not allocation counts.
- Docs, comment, test, fuzz, tla and python-only PRs are exempt.

## Building a train

1. Work on the steward's own branch only; never push to an owner branch, never force-push.
   `git fetch origin master`, `git checkout -B <steward-branch> origin/master`.
2. Fetch each owner branch by name (`git fetch origin <branch>`, one per call; a multi-argument
   fetch or `pull/N/head:prN` fails with "Needed a single revision"; use `git fetch -f` if a head
   moved). Merge the sha from FETCH_HEAD: `git merge --no-edit -m "Merge #N (task)" <sha>`.
3. Merge commits and messages end with the Co-Authored-By and Claude-Session trailers; the train
   PR body ends with the Generated-with line.
4. Train commit: set `status: done` and `pr:` (full https URL) in the task files the PRs close;
   multi-phase tasks stay in-progress. Run `python3 scripts/task_index.py`, then `--check`. Parity
   PRs: refresh the `docs/parity.md` table from `python3 test/parity/generate.py --report`
   (replace rows from `| area | cases` to `| **total`); parity PRs themselves never edit
   `parity.md` or `docs/tasks/README.md`. Leave the `docs/release-2.0.md` corpus line to the
   backlog thread.
5. Conflicts are mostly allow-lists (`test/parity/divergences/<area>.txt`,
   `python/tests/parity/module-divergences.txt`): drop the lines another PR removed, merge them
   keyed by case id, not by union. Never `checkout --ours` a whole `docs/parity.md`.
6. A train of one is normal when the base is stale, the PR overlaps a merged one (same `src/`
   file) or task statuses need flipping. If a new FINAL arrives within about 20 minutes of a
   train's start and touches the same files, restack; otherwise queue it behind the train.
7. Open the train PR as a draft, subscribe, mark it ready, merge with `merge_method` merge and the
   full 40-character `expectedHeadSha`, then unsubscribe. The merge message names perf
   exceptions, re-run gaps and any MSVC story.

## Waiting on CI (liveness)

- **Arm a `send_later` wake (30-45 minutes) whenever a train or PR you are merging is in CI, and
  re-arm it at every wake until it merges.** The 2026-11-21 stall: a green train sat 8.5 hours
  because no event woke the steward after the last job finished.
- Wait for `check_suite.completed` on the CURRENT head sha; events for older shas and echoes of
  your own ready-for-review need no reply.
- A runner-cancelled job (for example fuzz `render_wide` at about 45 minutes): `rerun_failed_jobs`
  once on that run id, merge only when its siblings pass, and record the gap in the merge message.
- A red job goes to `ci-triage` first; read its summary, not the log.
- Jobs now carry `timeout-minutes` (60; 90 for sanitizers and clang-tidy) and Windows ctest
  runs `--timeout 600`, so a hang fails in at most an hour. A new slow MSVC Debug test failing on
  that cap is the signal docs/tasks/0565 wanted.

## Traps

- `Value::ValueData` variant order is part of the ABI-visible layout (Markup, WMarkup, BigInt):
  a PR that adds an alternative must keep the order the others assume.
- Every PR adding `namespace jinja2` needs `python3 scripts/abi_namespace.py` run on the train.
- Tests that reopen `namespace jinja2 { namespace detail {` must use
  `template<> struct jinja2::detail::X<...>`.
- A type-level merge can break code with no textual conflict (the YAML writer used
  `MapRef::GetKeys`, removed by another PR), and two PRs green apart can be red together (one
  unblocks a case the other listed). Always build and test the train.
- clang-tidy flags implicit bool conversion in `&&` chains: use nested `if (p)` and `== 0`.
- `OrderedMap`/`TemplateDict` moves are conditionally noexcept (MSVC allocates). A PR adding
  noexcept moves over them aborts `jinja2cpp_alloc_tests` on Windows static jobs.
- MSVC: assigning `wchar_t` to `char` trips C4244 (use a `static_cast` loop); range-for over a
  braced list needs `<initializer_list>` (C3312, use a C array).
- PR CI builds the merge ref (PR plus current master), not the head. The perf job uses
  `pull_request.base.sha`, which can lag master: compare against the actual master.
- Local Python `unicodedata` differs from CI: do not commit regenerated `expected/output.json` repr
  lines. `FilesystemHandler` tests need ctest's cwd.
- GitHub MCP tools disconnect repeatedly: reload with ToolSearch `select:mcp__github__pull_request_read,
  update_pull_request,merge_pull_request,actions_run_trigger` (and `actions_get`).
- Task numbers are shared with all threads: check master and open PRs before filing one. #807,
  #284, #199 and #197 are not the steward's; leave them.

## After the merge

Message each owner (what is done, what stays open), unsubscribe, update the status checklist, and
run `python3 scripts/task_index.py --check` on the new master.

## Weekly rotation

Every Monday the steward writes a handoff to project memory (open trains and PRs, accepted perf
exceptions, anything pending with Ruslan), resolves its thread, and the coordinator starts a fresh
one. New traps go into this file, not into the handoff.
