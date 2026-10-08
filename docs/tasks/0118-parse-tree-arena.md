---
status: in-progress
priority: high
area: perf
depends: [0109]
touches: [src/template_parser.cpp, src/expression_parser.cpp, src/expression_evaluator.h, src/statements.h, src/statements.cpp, src/renderer.h, src/function_base.h, src/template_slots.h, src/template_impl.h, CMakeLists.txt, .github/workflows/linux-build.yml, src/node_arena.h, src/node_arena.cpp, src/filters.h, src/filters.cpp, src/filter_factories.cpp, src/testers.h, src/testers.cpp, src/tester_factories.cpp, src/render_context.h, src/expression_evaluator.cpp]
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
- **P4b** (template slots): a render links to the trees of the other templates it runs by
  `TemplateHandle` (src/template_slots.h), a 24-bit slot in the render's `TemplateSlots`
  table plus an 8-bit table generation; a handle of another render throws. Blocks on the
  inheritance stack and the parent an `extends` names are `{handle, NodeRef}`, 8 bytes,
  and the render's cache of loaded templates keeps them alive, so the parent, include and
  import holders (`shared_ptr` per extends or import) are gone. Templates that define no
  blocks and extend nothing share one empty block stack. Against master 8a2018d: Render
  inheritance -1.23% (213.5k, below pre-arena d47d436's 213.9k), plain_text -4.6%,
  substitute -1.8%, the rest -0.2..0%; Load -0.15..+0.30% from inlining changes in the
  unchanged parser code of that translation unit. `ErrorCode::TemplateExpired` and weak
  ownership wait for the first API that lets a callable outlive its render.
- **P4c-1** (filters and tests in the arena): the filter and test objects a template names
  live in its tree as `NodeRef<IExpressionFilter>`/`NodeRef<ITester>`. They are
  polymorphic, so `Seal` relocates them through their own vtable (`ArenaObjectBase`,
  wrapped as `detail::ArenaObject<F>` by `NodeArena::MakeObject`) under two kinds,
  `FilterObject` and `TesterObject`. The built-in filters and tests are sorted tables of
  factory pairs (heap for names chosen at render time, arena for names in the template) in
  src/filter_factories.cpp and src/tester_factories.cpp, apart from the bodies so that
  the factories do not crowd the filters out of inlining. Against master d333c43:
  many_tags 598 fewer allocations (1,560), Load -1.32%, Retained -14 KB; Load filters
  -2.97%, chat_llama -0.93%; Render -0.11..+0.21% except dict_ops +0.52%. The Load rises
  on cases with no filters (substitute +2.06%, for_range +1.08%, inheritance +1.02%) and
  dict_ops' Render are all in `_int_malloc`/`malloc_consolidate` (callgrind, no other
  function changes): the heap no longer holds the tester map built at static init.
- **P4c-2** (node reference checks): `JINJA2CPP_NODEREF_CHECKS` is `OFF`, `ON` (Release
  default) or `FULL` (Debug, sanitizer and fuzz default), passed to every target that
  includes src/ through the library. `Seal` checks each link as it moves the node that
  holds it (`VisitRefs` on every node class, `detail::RefChecker`): inside the tree, at a
  node's start (a bitmap built from the cleanup table), of its kind family. Lists are
  checked to lie inside the tree. A failure is `InvalidNodeRef`, which Load reports as
  `UnexpectedException` while keeping the previous template. Links into another
  template's tree are checked where a render follows them (`TemplateSlots::Resolve`).
  `FULL` checks every access and requires that `Seal` reached the node, so a link that
  `VisitRefs` misses fails the first test that follows it. Four Linux matrix rows build
  with the checks `OFF`. Against P4c-1: Load many_tags +2.66%, small templates
  +200..330 instructions (about 20 per link, against the plan's 6-8); Render unchanged
  except inheritance +0.10% (`Resolve`). P5 carries this cost under its small-template
  Load target.
- P5b-1 (destroy only what owns something): every leaf node
  class is `final`; `ExpressionEvaluatorBase`, `IRendererBase` and the bases with subclasses
  keep a protected non-virtual destructor, so a node that owns nothing is trivially
  destructible (`IsTriviallyDestructibleNode`, which probes a protected destructor through a
  final subclass) and the arena never destroys it: its `destroy` op is null, `MoveNodes` writes
  the cleanup table of the owners only, and Seal's second pass and the template's
  destruction walk that table. The bitmap of node starts lives in the sealed buffer, so
  `ArenaView::Checked` tests a bit instead of searching the cleanup table, and Seal makes no
  heap bitmap for large trees. `LookupCache::Forget` and `~ValueRefExpression` are gone:
  each render takes a lookup epoch of its own and keeps every tree it runs alive, so a
  tree at a freed tree's addresses is never seen under the freed one's epoch (tests
  `LookupCacheIgnoresAFreedTemplatesEntries`, also across threads). Epochs are unique
  within a thread's cache, and a context is used only on the thread whose cache it holds
  (a Debug assertion on every cached lookup); escaped callables must run in a context of
  their own (constraint in the design plan's Escapes). Against 4fca3f9: Load
  -0.2..-1.1% (plain_text -1.09%, substitute -0.99%, many_tags -0.79%, one allocation fewer
  on large templates), Render unchanged. Most node kinds still own a name, a constant or a
  vector; P5b-2 (constants, keep-alives) and P5b-3 (names, lists) make them trivial.
- P5b-2a (template root and raw text own nothing): `TemplateRenderer` keeps its blocks as an
  `ArenaSpan` of `BlockStatement` refs, collected during the parse (`TemplateRootInfo`, which
  rejects a second block of a name, comparing names one by one up to 16 blocks and through
  a set past them) and set when it ends.
  `RawTextRenderer` drops its `shared_ptr` holder: text converted to `newline_sequence` lives in
  a list the template owns next to its source. Both kinds are now trivially destructible.
  Against P5b-1: Load plain_text -3.05% (-4.11% against 4fca3f9), inheritance -4.01%,
  for_filter_if -2.03%, substitute -1.07%, the rest within +0.32%; Render within 0.35%;
  retained plain_text 752 to 688 B, inheritance 1,960 to 1,632 B.
