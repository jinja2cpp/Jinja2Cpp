---
status: in-progress
priority: high
area: agents
---
# Agent roles with per-role model and effort

**Problem.** One agent configuration for every task wastes capacity in both directions:
a strong model spent on grepping is expensive, a cheap model making architectural calls
is unreliable. Like a workshop, the work splits into scouting, design and inspection,
each needing a different kind of attention.

**Proposal.** Role subagents in `.claude/agents/`, each with its own model, effort and tools:

| role | job | model / effort |
|---|---|---|
| `explorer` | find code, map call paths, summarise; read-only | small model |
| `architect` | design changes, weigh trade-offs, write plans; read-only | strongest model, high effort |
| `implementer` | make a scoped change with a test, run the build | mid model |
| `verifier` | adversarially check a diff: build, tests, Python oracle, sanitizers | mid model, high effort |
| `parity-checker` | compare a template's output with Python Jinja2 | small model |

First cut (this task): the definitions above plus a CLAUDE.md section describing when the
main session should hand work to each role.

Follow-ups:
- Measure: run the same tasks with and without roles and compare cost and rework.
- Orchestration recipes for common flows (bug report → explorer → implementer →
  verifier; feature → architect → implementer → verifier).
- Review conventions: the verifier's checklist becomes the PR review checklist.

**Done when.** Roles are used in practice on at least a few PRs and the recipes are
written down from that experience rather than up front.

**Next.** Role boundaries will leak (an implementer needing design decisions); the
measured rework rate tells where to move them.
