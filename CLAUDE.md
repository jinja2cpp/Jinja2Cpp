# Jinja2C++ — Claude Code instructions

The shared project instructions (build/test commands, layout, behavior and style
rules, task conventions and PR workflow) live in AGENTS.md and are imported here:

@AGENTS.md

Before reviewing any PR, also read [REVIEW.md](REVIEW.md). Keep shared rules in
AGENTS.md and review criteria in REVIEW.md rather than duplicating them here.

This file adds Claude Code-specific environment setup, agent roles and coordination.
Other runners should use their own available tools and environment initialization.

## Claude Code environment

In Claude Code on the web the SessionStart hook (`.claude/hooks/session-start.sh`)
configures and builds `build/`. Project threads can open in the parent directory,
where the hook does not fire: inspect the checkout first; if setup is missing, run
`CLAUDE_CODE_REMOTE=true .claude/hooks/session-start.sh` from the repository root
and export the `JINJA2CPP_CMAKE_INIT` it prints.

When archive downloads are blocked in that runner, the hook supplies an initial
cache for new build directories:
`cmake -S . -B build-x -G Ninja -C "$JINJA2CPP_CMAKE_INIT" ...`.
In Claude Code on the web, `-DJINJA2CPP_WITH_JSON_BINDINGS=rapid` still needs an
archive download (patched RapidJSON) and does not configure, and ConanCenter is
blocked, so Conan builds run only in CI.
Keep strict warnings and TLS/checksum verification enabled, as in AGENTS.md.

## Agent roles

`.claude/agents/` defines role subagents, each with a model and effort sized to the job
(docs/tasks/0004). The recipes below say when each is worth its cost:

| Role | Use for | Model | Where it runs |
|---|---|---|---|
| `explorer` | locating code, tracing call paths, finding covering tests, reading anything large (cheap) | Haiku | read-only, shared checkout |
| `ci-triage` | a red CI job: failing step, cause class, the lines that matter | Haiku, low effort | read-only |
| `architect` | cross-module or public-API design; returns a plan | Opus, high effort | read-only, shared checkout |
| `advisor` | a second opinion on the triggers below; returns a decision memo | Fable, high effort | read-only, shared checkout |
| `implementer` | one scoped change plus its test, built, run and committed | Sonnet | own worktree and `build/` |
| `verifier` | adversarial pre-push check: build, tests, sanitizers, Python oracle | Sonnet, high effort | own worktree and build dirs |
| `parity-checker` | rendering the same templates with Jinja2C++ and Python Jinja2 | Haiku | read-only, scratch dir |

`implementer` and `verifier` have `isolation: worktree`: each call gets a fresh checkout
under `.claude/worktrees/` made from the caller's **last commit** (commit before
delegating; uncommitted edits are invisible to them) and builds there, so a sanitizer
build or a second implementer never clobbers `build/`. In the cloud the SessionStart
hook sets up ccache so a worktree build reuses `build/`'s objects. An implementer hands
back a commit on its worktree branch; bring it in with `git cherry-pick <sha>`.

Recipes (corrected from PRs #293-#345, measured in docs/tasks/0004):
- Bug report or one feature: the main session explores and implements; the verifier
  runs on the committed result before the first push of any `src/` or `include/` change.
  One verifier round found a real bug in 8 of the 17 PRs that ran it.
- Cross-module design, value-model or public-API change (and tasks whose file asks for
  a plan): architect first, then as above.
- Mechanical batches (clang-tidy fix-its, renames, reformatting) and docs: no verifier;
  `scripts/preflight.sh` plus CI are the check.
- Once the design is settled, an Opus thread hands the edit-build-test loop to
  `implementer` (Sonnet) with a precise brief: files, expected behaviour, the test rows
  to add. Compiler errors, test output and file reads then stay out of the Opus
  context, which every later wake re-reads; the thread reviews the returned commit and
  the verifier checks it as usual. Also use it for a second, independent change in
  different files that should land in the same PR; parallel tasks are separate project
  threads. `explorer` and `parity-checker` are for sweeps too wide to read inline.
- Review: follow [REVIEW.md](REVIEW.md); the verifier checklist adds executable
  pre-push checks. Run it before marking an implementation PR ready and paste its
  verdict into the PR conversation.
- Before every push of `src/`, `include/` or `test/`: `scripts/preflight.sh` (add
  `--perf` when `src/` changed). It runs CI's changed-line checks as CI runs them; 12 of
  the 19 red-push causes in #346-#445 were those checks run locally in a narrower form.
- A red CI event: hand the job to `ci-triage` first and read its summary, not the log.
  A log read into the main session is paid for again on every later wake.
- `advisor` (expensive: about $3-8 a consult) only when one of these holds: the verifier
  reports blocking findings two rounds running; two red pushes share a cause; a perf
  phase lands at less than half its planned gain or regresses past its gate; a
  value-model, arena or public-API plan is about to go to Ruslan. Give it a written
  brief (decision, evidence, what was tried, file pointers), not the conversation.

The cloud cannot build with MSVC or Apple Clang. MSVC is the largest cause of red pushes
that preflight cannot catch (5 of 19 in #346-#445); the verifier's platform items are the
cheap guard, CI the real one.

In the PR description (the body, not a comment, so it can be collected; trains too),
note which roles ran, how many verifier rounds it took and how many pushes went red in
CI, with the cause of each red push.

## External review (Codex)

Codex (`chatgpt-codex-connector[bot]`) reviews PRs against REVIEW.md. Codex settings
start it automatically only when a PR is opened for review or marked ready, and our
PRs open as drafts, so owner threads ask for it explicitly:

- When a PR is green and before telling the merge steward it is final, its owner
  thread comments `@codex review` once and waits for the result (a 👍 reaction or a
  review). Scope: changes in `src/`, `include/`, `test/`, `cmake/`, `thirdparty/`,
  `CMakeLists.txt`, `.github/workflows/` or the agent instructions (AGENTS.md,
  CLAUDE.md, REVIEW.md, `.claude/`). Skip docs-only and task-only PRs, bench
  re-measures, mechanical clang-tidy batches and merge-steward train PRs. Add
  `@codex security review` when the diff touches the lexer, parsers, evaluator,
  loaders or escaping.
- Re-request only after a substantive change (new logic, not nit fixes), at most
  twice per PR.
- Triage: P0/P1 are blocking like a red-circle finding (fix, or reply with the traced
  reason it is wrong); P2 is a claim to verify like any bot finding; P3 is optional
  (one-line reply). Reply on each thread and resolve the ones you fixed.
- Add `Codex: N findings, M fixed, K declined` next to the verifier line in the PR
  body.
- The merge steward checks before stacking a train that each in-scope PR has a Codex
  result with no unanswered P0/P1 thread. If Codex never answered, the PR still goes
  in and the gap is noted in the train PR, so a Codex outage never blocks merges.
- Codex model and effort are set in Codex's own settings, not here; Claude role
  frontmatter does not affect them.

## Batching work

Parallelism comes at three levels; pick the outermost one that fits.

1. **Project threads** (one task per thread). Each thread has its own container,
   clone, `build/`, branch and PR, and costs a cold start (SessionStart hook, about two
   minutes). Use a separate thread for each task in `docs/tasks/` and for anything that
   should be its own PR with its own review and CI. Run
   `python3 scripts/task_batches.py` to see which tasks can run side by side: tasks in
   one wave have disjoint `touches`. Tasks that overlap run one after another, or one
   thread owns the shared files and the others send it their changes.
2. **Subagents in one thread.** Read-only roles (explorer, architect, parity-checker)
   fan out freely: launch them in one message. Implementers run in parallel only when
   their changes touch different files; each gets its own worktree, so the risk is
   merge conflicts on cherry-pick, not corrupted builds. A container has 4 cores: at
   most two concurrent builds, each with `--parallel 2`. The verifier runs last, on the
   committed result.
3. **Tool calls.** Independent reads, searches and commands go in one message.

**Thread lifetime.** Every wake re-reads the whole conversation, so cost grows with a
thread's age: cache reads and writes were 86% of measured spend, and four long-lived
threads 74% of it (docs/tasks/0157). A thread that finishes a phase of a multi-phase
task, or whose context passes about 300k tokens, writes its handoff to project memory
(state, open PRs, decisions, traps), resolves, and the coordinator starts a fresh thread
for the next phase. A multi-phase task also rotates at plan approval: the architect's
plan and Ruslan's decision are the handoff, and the coordinator starts the implementation
thread then (the 0038 plan thread ran 22 hours for $86, docs/tasks/0157). Standing threads (merge steward, tracks) rotate the same way once
per wave or week. Mechanical threads (clang-tidy batches, index or docs syncs, bench
re-measures, CI tweaks) and the merge steward run on Sonnet; behaviour and perf work
stays on Opus.

Keep work in one thread when the pieces must land in one PR, or when each step needs
the previous one's output (explore → design → implement is a pipeline, not a batch).
Split into threads when the pieces could merge in any order.
