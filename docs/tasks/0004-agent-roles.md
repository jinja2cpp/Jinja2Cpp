---
status: in-progress
priority: high
area: agents
touches: [.claude/, CLAUDE.md, scripts/task_batches.py, docs/tasks/README.md, docs/tasks/0004-agent-roles.md, .gitignore]
pr: [https://github.com/jinja2cpp/Jinja2Cpp/pull/291]
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

Follow-ups:
- Fill in `touches` for the other task files once the 0001 parity-map thread has
  settled the registry (it is editing `docs/tasks/` now).
- After a handful of PRs, read the measurement lines back: where verifier rounds or red
  pushes cluster, move the role boundary or change the role's model.

**Done when.** Roles and worktree isolation are used on at least a few PRs, the
measurement lines from those PRs are summarised here, and the recipes in CLAUDE.md have
been corrected from that experience.

**Next.** Role boundaries will leak (an implementer needing design decisions); the
measured rework rate tells where to move them. `touches` is a forecast: tasks that
regularly edit more than they declared point at hidden coupling in the code (a header
everyone includes, a test table everyone extends) that is worth splitting.
