---
status: in-progress
priority: high
area: perf
depends: [0109]
touches: [src/template_parser.cpp, src/expression_parser.cpp, src/expression_evaluator.h, src/statements.h]
---
# Allocate a template's parse tree from one arena

**Problem.** After 0103 a tag-heavy `Load` still makes about 16.5k allocations
(`Load/many_tags`), most of them expression and statement nodes held by
`shared_ptr`; allocator calls are about a quarter of Load and destroying the template
another 9%. The nodes live exactly as long as the template, which is what an arena is
for, but `shared_ptr` ownership lets a node outlive its template (macros, callables
captured into values) and that has to stay safe.

**Proposal.** Architect plan first: a monotonic arena owned by the template impl, nodes
allocated from it, with the escape cases (a node referenced after the template dies)
found and either copied out or made to keep the arena alive. Measure against 0109's
smaller wins first.

**Done when.** `Load/many_tags` -30% instructions over 0109's result, sanitizer and
fuzz runs clean.

**Plan (approved by Ruslan 2026-10-06).** docs/design/0118-parse-tree-arena-plan.md, phases sequenced with 0118/0117,
0130, 0131 and 0137-0142 in docs/design/perf-design-overview.md section 5.

**Progress.**
- **P3** (typed handles): nodes are made by a `NodeArena` owned by the template and linked
  by `NodeRef<T>` and `ArenaSpan<T>` (src/node_arena.h); each node has a `NodeKind` byte,
  so downcasts are kind checks (`Is`/`As`/`Get`) instead of `dynamic_cast`. Handles still
  wrap pointers; nodes are separate heap objects whose destroy records and child lists are
  bump-allocated from doubling blocks. Deferred to P4/P5 with the arena and side tables:
  pre-order placement, the hot/cold split, `SymbolId`/`ConstRef`, call parameters,
  macro parameters and `with` variables as spans, `CreateFilter`/`CreateTester` taking
  the arena, offsets and their checks. Instructions against decision 5 (PR #425): Load
  -7.1..-0.8% on all cases but `substitute` +2.2% and `plain_text` +7.1% (the first
  512 B block, about 290 instructions, gone with P4's pre-sized buffer); Render
  -1.5..+0.1%.
