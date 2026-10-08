---
status: open
priority: low
area: perf
depends: [0117, 0118]
touches: [src/expression_evaluator.h, src/expression_evaluator.cpp, src/name_resolver.cpp, src/render_context.h]
---
# Skip the lookup cache for names that are read once per render

**Problem.** The lookup cache keys an entry by the name node and the render's epoch, and
every render takes a new epoch (0118 P5b deleted the cache's `Forget`). A name node
outside any loop, macro body or call block runs at most once per render, so it always
misses: the cache only costs it the probe and the insert. After 0118 P5b-3a a miss also
reads the name through the tree's view, 3 instructions more than before (a hit 3 fewer).
Render/large_static (50 `{{ title }}` nodes, all outside loops) rose 0.55%, inheritance
0.43%; the perf track accepted that as a miss/hit trade (suite Render -0.16%) on condition
that this task is filed.

**Proposal.** The name resolver knows at parse time whether a use sits inside a loop, a
macro or a call body (its frames). Set a flag on `ValueRefExpression` for the uses that do
not, and let them go straight to `FindValue`, skipping the cache slot (and its `NewSlot`).
Measure Render on flat templates (large_static, substitute, config_file, inheritance), and
check that loops do not lose: a macro body is re-entered per call and an included template
per include, so those stay cached.

**Done when** the flat cases' Render falls by at least the P5b-3a miss cost and no case
rises past 0.3%.
