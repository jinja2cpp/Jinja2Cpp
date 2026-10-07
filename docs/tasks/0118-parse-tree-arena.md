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
  wrap pointers; each node is constructed in a bump-allocated block right after its
  destroy record, and child lists share those blocks (doubling from 512 B to 64 KB), which
  brings P4's allocation saving forward. Deferred to P4/P5 with the arena and side tables:
  pre-order placement, the hot/cold split, `SymbolId`/`ConstRef`, call parameters,
  macro parameters and `with` variables as spans, `CreateFilter`/`CreateTester` taking
  the arena, offsets and their checks. Instructions against decision 5 (PR #425): Load
  -25..-4% on every case (many_tags and html_autoescape -25%, plain_text -4.1%,
  substitute -6.4%); Render -1.6..+0.3%.
- **P4a** (exact buffer, 32-bit offsets): `NodeRef<T>`/`ArenaSpan<T>` are offsets into the
  template's tree. The parse still bump-allocates into blocks; `NodeArena::Seal` copies
  them into one buffer the size of what they hold, move-constructs each node over its copy
  (a kind→ops table), lets `ForStatement` repoint its loop-name views, and keeps a 4-byte
  cleanup entry per node. `RenderContext` carries the running template's `ArenaView`;
  `ArenaSwitch` installs another template's view where its code runs (include, parent,
  blocks, macro and caller callables, `loop(...)`, lazy filtered loops). A body's
  children follow its `ComposedRenderer` in the same allocation. Against master d47d436:
  Retained below on every case (many_tags 562 KB, -30%); Render -0.3..+1.65% (offset
  base load per access, accepted by the Performance track; inheritance +1.1% back to P4b,
  substitute/plain_text fixed cost to P5); Load +6.3..+12.9% from the per-node move at
  Seal, accepted by Ruslan until P5 makes nodes trivially copyable.
