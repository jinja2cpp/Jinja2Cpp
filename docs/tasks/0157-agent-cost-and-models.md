---
status: in-progress
priority: medium
area: agents
depends: [0004]
touches: [.claude/agents/, CLAUDE.md, scripts/preflight.sh, scripts/task_index.py, docs/tasks/README.md, .github/workflows/format-check.yml]
---
# Agent spend goes to re-reading long threads, not to the model that runs them

**Problem.** Two pressures pull on the agent workflow. Quality wants the strongest model
on every step and a thread that remembers everything; cost and speed want cheap models
and short contexts. Measured on 25 project sessions (2026-10-10, $1,137 at list prices),
the second pressure is not where it was assumed to be: 86% of spend is cache reads and
writes and 14% is output. Every wake re-reads the whole conversation, so a thread's cost
grows with its age, and four long-lived threads (the wave 2 arena thread $355, the merge
steward $176, the performance track $155, the remaining clang-tidy thread $149) were 74%
of the total. The steward's output was 5% of its cost: it pays $3-6 per wake to re-cache
370k-740k tokens in order to merge a PR or refresh a status line.

| Session kind | Sessions | Spend | Cache write / read / output |
|---|---|---|---|
| Multi-phase task threads | 2 | $504 | 38 / 46 / 16% |
| Standing tracks | 2 | $197 | 51 / 39 / 13% |
| Merge steward | 1 | $176 | 55 / 40 / 5% |
| One-task threads (parity, perf, API, bench) | 17 | $228 | 22-49 / 41-64 / 12-23% |
| Mechanical, docs and CI threads | 3 | $32 | 23-54 / 37-64 / 12-18% |

Opus was 85% of spend; the verifier and other Sonnet subagents about 18%.

Role use and red pushes in PRs #346-#445 (99 PRs, read from the PR bodies): the
verifier ran on 63 of 74 code PRs (76 rounds) and found a real defect on 23 (37%); the
architect ran on 13; explorer on 1; implementer and parity-checker on none. 18 pushes
went red across 15 PRs. Causes: MSVC 5, instruction-count gate 4, clang-tidy 4, CodeQL
`cpp/suspicious-add-sizeof` 3, fuzz (pre-existing bug) 2, clang-format 1. Twelve of the
19 were checks the thread had run locally in a narrower form than CI (tidy over `.cpp`
only, tidy not re-run after a fix, no gate run, no CodeQL). Apple Clang and macOS caused
none, unlike #293-#345 (0004). Ten PRs were docs-only task-index syncs, and the index
had still drifted (0153 shown open, 0155 and 0156 missing).

**Proposal.** Resolve the contradiction where the money is, then match models to roles:

1. *Thread lifetime* (CLAUDE.md "Batching work"): a thread that finishes a phase, or
   whose context passes about 300k tokens, writes a memory handoff and resolves; the
   coordinator starts a fresh one. Standing threads rotate once per wave or week.
   Expected: about 40% of long-thread spend.
2. *Models by role*: the merge steward and mechanical threads on Sonnet 5.5 (project
   settings and the coordinator, not the repo); behaviour and perf threads stay on Opus
   5.5; the verifier stays on Sonnet 5.5 at high effort (its hit rate is the best value
   in the workflow); explorer and parity-checker on Haiku (now 5.5).
3. *`scripts/preflight.sh`*: CI's changed-line checks run the way CI runs them, before
   every push; `--perf` adds the instruction gate. The verifier calls it.
4. *`ci-triage`* (Haiku, low effort): reads a failed job's log and returns the cause
   class and the lines that matter, so the log never enters an Opus context.
5. *`advisor`* (Fable 5.1, high effort): a second opinion on explicit triggers (blocking
   verifier findings two rounds running, two red pushes with one cause, a perf phase
   below half its plan, a value-model, arena or public-API plan before it goes to Ruslan).
   About $3-8 a consult; Fable costs 2.5 times Opus per token.
6. *`scripts/task_index.py`*: the README index is generated from the task files; PRs do
   not edit it, the steward regenerates it after a merge batch, the format workflow warns
   on drift.

Haiku 5.5 is not used for implementing or verifying: it scores 39.2% on Terminal-Bench
4.0 against 70.6% for Sonnet 5.5, and Anthropic points complex agentic coding at Sonnet
or Opus.

**How to re-measure.** Per-session spend: `list_events` (claude-code-remote) with
`kinds: ["result"]` on a session; the newest result event's
`internal_anthropic_catchall.modelUsage` is cumulative per model (costUSD, cache write
and read tokens, output tokens). Roles and red pushes: the measurement lines in PR
bodies.

**Done when.** The roles, scripts and rules above are merged; the steward and
mechanical threads run on Sonnet; and a re-measure over the next 50 PRs and the threads
that made them shows spend per merged PR and red pushes per PR against the figures here.

**Next.** Shorter threads move cost into handoffs: a handoff that misses a decision
shows up as a re-asked question or a repeated mistake, which memory quality, not model
choice, then has to fix. Preflight will catch less as CI grows checks it does not mirror;
keep it in step with `.github/workflows/`.
