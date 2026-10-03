---
status: done
priority: high
area: agents
touches: [.claude/, CLAUDE.md, scripts/task_batches.py, docs/tasks/README.md, docs/tasks/0004-agent-roles.md, .gitignore]
pr: [https://github.com/jinja2cpp/Jinja2Cpp/pull/291, https://github.com/jinja2cpp/Jinja2Cpp/pull/293]
---
# Agent roles with per-role model and effort

**Problem.** One agent configuration for every task wastes capacity in both directions:
a strong model spent on grepping is expensive, a cheap model making architectural calls
is unreliable. Like a workshop, the work splits into scouting, design and inspection,
each needing a different kind of attention.

A second tension appears once roles exist: parallel agents are only useful if they do
not trip over each other. Two implementers in one checkout overwrite each other's files,
a sanitizer build in `build/` wipes the warm Debug build, and two project threads
editing the same files end in a rebase. Parallelism has to be planned around what each
piece of work touches.

**Proposal.** Role subagents in `.claude/agents/`, each with its own model, effort and tools:

| role | job | model / effort | where it runs |
|---|---|---|---|
| `explorer` | find code, map call paths, summarise; read-only | small model | shared checkout |
| `architect` | design changes, weigh trade-offs, write plans; read-only | strongest model, high effort | shared checkout |
| `implementer` | make a scoped change with a test, run the build, commit | mid model | own worktree |
| `verifier` | adversarially check a diff: build, tests, Python oracle, sanitizers | mid model, high effort | own worktree |
| `parity-checker` | compare a template's output with Python Jinja2 | small model | scratch dir |

1. *Roles* (done in #291): the definitions above plus a CLAUDE.md section describing
   when the main session hands work to each role.
2. *Isolation*: writing roles get `isolation: worktree`, so each runs in a fresh
   checkout of the caller's last commit with its own build directories. ccache, set up
   by the SessionStart hook with `base_dir` at the project root, lets those builds reuse
   the objects already compiled in `build/`; dependencies are unpacked once into a
   shared read-only source cache and built per tree, so concurrent configures do not
   clobber each other.
3. *Planning parallel work*: task files declare `touches:` (paths the work edits).
   `scripts/task_batches.py` groups active tasks into waves with disjoint `touches` and
   satisfied `depends`; each task in a wave can run as its own project thread
   (container, branch, PR). CLAUDE.md "Batching work" says when to use threads,
   subagents or plain parallel tool calls.
4. *Recipes and review*: bug, feature and parity-batch flows in CLAUDE.md; the
   verifier's checklist doubles as the PR review checklist.
5. *Measurement*: each agent-driven PR notes which roles ran, the number of verifier
   rounds and the number of pushes that went red in CI.

Follow-ups (done in the closing PR): every active task file now declares `touches`, and
the measurement lines were read back:

**Measurements, PRs #293-#345 (55 merged PRs, read 2026-10-03).** 46 carry a measurement
line; the 9 without are three docs PRs, three integration trains and the three PRs that
predate the convention (#291, #292, #294). Three more put their numbers in PR comments.

| role | PRs that ran it | what it did |
|---|---|---|
| `verifier` | 17 | a real bug in 8 of them on the first round (stack-use-after-return #320, signed overflow in slice stepping #306, quadratic splitter #316, invalid UTF-8 #297, default-scope regressions #298, false rejections #312, 2-3 blockers #322, #334); 1 PR needed a second round |
| `architect` | 4 (#309, #313, #318, #322) | plans for the four tasks that required one; the 2.0 API design (#328) was done in the main session |
| `implementer` | 0 | every change was made in the main session |
| `explorer`, `parity-checker` | 0 | exploration was done inline; parity checks ran through the corpus |

Red pushes: 11 across 9 PRs, 33 PRs had none. Causes: MSVC 4 (#318, #332, #335, #336),
clang-format 2 (#317, #320), macOS locale-dependent wide-string cases 2 (#307, #317),
Apple Clang `-Wunused-private-field` 1 (#306), JSON binding tests outside the default
configuration 1 (#304), and one real RapidJSON metadata bug CI caught (#335). PRs that
ran the verifier went red more often (6 of 17) than the rest, being the larger behaviour
changes; 7 of the 11 red pushes come from compilers the cloud cannot run, which a
verifier round cannot build. Runner losses (#321, #324, #341) are not counted.
Ruslan's reviews still found Python-parity gaps the verifier missed, mostly inputs
Jinja2 rejects (#297, #298, #301, #305, #307).

Worktree isolation worked when used (ccache 63/63 hits, a fresh worktree builds in about
12 s instead of 100 s, #293), but only the verifier used it. `touches` needed `shares`
and `path#region` before it could schedule the parity race (#295: 22 tasks in 20 waves,
6 after the fix), and trains still hit "green + green = red" (#302, #319, #326), which
the merge steward absorbs.

**Corrections made.**
- Recipes in CLAUDE.md now say what the data shows: the main session implements, the
  verifier is the role that pays for itself and runs before the first push of any
  `src/`/`include/` change, mechanical batches skip it, and `implementer` is only for a
  second independent change in the same PR (parallel tasks are project threads).
- The verifier checklist gained the platform hazards behind the red pushes and a check
  that inputs Jinja2 rejects are rejected too.
- The measurement line goes in the PR body, trains included, with each red push's cause.
- Models are unchanged: no failure traced back to a role's model. The unused roles stay;
  they cost nothing until called.

**Done when.** Roles and worktree isolation are used on at least a few PRs, the
measurement lines from those PRs are summarised here, and the recipes in CLAUDE.md have
been corrected from that experience.

**Next.** Role boundaries will leak (an implementer needing design decisions); the
measured rework rate tells where to move them. `touches` is a forecast: tasks that
regularly edit more than they declared point at hidden coupling in the code (a header
everyone includes, a test table everyone extends) that is worth splitting.
