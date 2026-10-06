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

**Progress.**
- P0 (an imported macro's own names hide its module's names): #419.
- P1a (`LookupResult` and `MutableLookupResult` in src/lookup_result.h replace the
  `const InternalValue*` of `EvaluateRef`/`FindValueCached`, the entry pointer and `bool&` of
  `FindValue` and the pointer of `FindValueSlot`, now `FindForWrite`): this PR. Render
  instructions -0.8..+0.4% against master 5ab1e4a, inside the ±0.5% the plan allows. Unlike the
  plan's sketch, the constructor from a reference is public (explicit): constant nodes return
  their own value. Found on the way: task 0144 (`loop.cycle` is the integer 2).
- P1-i (`loop.<attribute>` resolved at Load to a `LoopAttr` in a `LoopAttrExpression`, read
  through `IMapAccessor::GetLoopAttr`; a `loop` that is not a for loop's falls back to the
  name lookup): this PR. Render instructions against 0118 P3: for_loop_vars -53.9% (502
  allocations down to 2), html_autoescape -5.0%, strings -4.4%, chat templates -1.7..-2.2%,
  other cases ±0.2%; Load within ±0.7%.
