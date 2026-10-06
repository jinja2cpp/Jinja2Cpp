---
status: in-progress
priority: high
area: perf
depends: [0038]
touches: [src/render_context.h, src/expression_evaluator.cpp#ValueRefExpression, src/statements.cpp, src/template_parser.cpp]
---
# Resolve variable names to slots at Load

**Problem.** Every name in a template is looked up by hash through a stack of
robin_hood maps at render time. 0100 added a thread-local lookup cache (epoch-keyed),
which halves the cost on hits, but `FindValueCached` plus `ValueRefExpression` are still
6-13% of `mitsuhiko_table`, `many_tags` and `expressions`, and every scope is a hash
map that must be built, cleared and kept node-stable. Python compiles each name to a
local variable slot. 0088 deferred this until lexical scoping (0038) settles what a
name can refer to.

**Proposal.** Architect plan first. Resolve, at Load, each name to (frame depth, slot)
where the scoping rules allow it (loop variables, `set` in the same block, macro
parameters), and fall back to the dynamic lookup for names that can come from the
context, includes or `globals`. Scopes become small arrays for the resolved part.

**Done when.** A plan with measured estimates is reviewed; implementation shows
-10% or better on `mitsuhiko_table` and `expressions`.

**Next.** With slots, the lookup cache (0100) and most scope maps can go.

**Plan (approved by Ruslan 2026-10-06).** docs/design/0117-name-slots-plan.md, phases sequenced with 0118/0117,
0130, 0131 and 0137-0142 in docs/design/perf-design-overview.md section 5.
