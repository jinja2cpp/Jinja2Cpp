---
status: in-progress
priority: high
area: perf
depends: [0038]
touches: [src/render_context.h, src/name_resolver.h, src/name_resolver.cpp, src/statements.h, src/expression_evaluator.cpp#ValueRefExpression, src/statements.cpp, src/template_parser.cpp]
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
- P1-ii to P1-iv (slot frames from a per-thread `RenderWorkspace`, the Load-time
  `NameResolver`, loops binding `loop` and their targets in slots with a frame view for the
  lookups by name, a Debug and `-DJINJA2CPP_CHECK_SLOTS=ON` cross-check of every slot read):
  this PR. Against 0118 P3 with P1-i, Render: for_filter_if -10.9%, mitsuhiko_table -5.5%,
  chat templates -1.6..-3.9%, strings -3.5%, for_range -2.0%. Named regressions: Render
  inheritance +5.5% (an include in a loop walks the loop's view out of line on every lookup
  cache miss; the lookup cache rekey of phase 6 removes it), plain_text, substitute and
  large_static +2..+2.8% (a larger RenderContext and the unit-call check; P5's ScopeMap and
  RenderContext slimming), macros +1.7% (same walk as inheritance); Load +0.3..+4.1% (the
  resolver: about 100 instructions per template and 400-500 per loop). 38 parity cases pin
  the scoping the resolver keeps; all render as before.
- P2 (macro and call-block arguments, then the `caller`, `kwargs` and `varargs` the body
  catches, in the first slots of the unit's frame; loops in the body start after them; a `set`
  of an argument in the body, or a nested macro or call body reading it, keeps the lookup by
  name): this PR. Against 3cc8e7d, Render: macros -13.5%, config_file -4.4%, html_autoescape
  -4.1%, other cases ±0.0%. Below the plan's -20..-25% on macros: what remains of a call is
  copying the arguments out of the `const CallParams&` (about 230 instructions per call in
  variant and shared_ptr copies), taking and giving back the frame (about 100) and the argument
  checks (about 140). Named regression: Load macros +2.25% (the resolver now walks the macro
  frame for each name, about 1,900 instructions for this template), inside the Load budget of
  0118 P5. `with` targets and block `super` stay lookups by name: no bench case uses them; they
  move to P4 with the root slots.
