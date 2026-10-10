---
status: in-progress
priority: high
area: perf
depends: [0109]
touches: [src/template_parser.cpp, src/expression_parser.cpp, src/expression_evaluator.h, src/statements.h, src/statements.cpp, src/renderer.h, src/function_base.h, src/template_slots.h, src/template_impl.h, CMakeLists.txt, .github/workflows/linux-build.yml, src/node_arena.h, src/node_arena.cpp, src/filters.h, src/filters.cpp, src/filter_factories.cpp, src/testers.h, src/testers.cpp, src/tester_factories.cpp, src/render_context.h, src/expression_evaluator.cpp, src/template_parser.h, src/name_resolver.h, src/name_resolver.cpp, src/slot_frame.h, src/internal_value.h, src/robin_hood.h]
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
- P5b-2b (parse scaffolding of small templates): the parser's stack of open statements is a
  `small_vector` of four instead of a `std::list` (no allocation for the root), and the
  arena's inline block and offset list are no longer zero-filled. Against P5b-2a: Load
  plain_text -6.53% (4,038, under the 4,121 target), substitute -2.10%, for_range -1.53%,
  for_filter_if -0.98%, inheritance -0.90%, the rest -0.66..+0.46% (chat_llama +0.40%,
  large_static +0.46%); Render unchanged. A `small_vector` for the text blocks too saved one
  more allocation (plain_text -10%) but its `emplace_back` cost large templates up to +1.3%,
  so they stay a reserved `std::vector`.
- Constants and keep-alives in side tables of the sealed buffer (the plan's P5b-2) were built
  and measured against P5b-2a and set aside: Load +0.3..+7% (expressions +7.0%, plain_text
  +4.2%: the empty tables' fixed cost in Seal and destruction, the parse table's growth) and
  Render up to +1.7% (expressions: a constant read through the view costs about 7
  instructions more than one in its node). A constant's value costs the same to move and
  destroy wherever it lives, so the table saves only the per-node dispatch. Left for P5b-3:
  scalar literals kept in the node as trivially copyable data, which no table can beat.
- P5b-3a (names, call arguments, comparisons and scalar literals in the tree): variable names
  and a filter's argument error are `ArenaText`s (an `ArenaSpan<char>`), a call's arguments
  and a comparison chain's operands arena lists, and a None/bool/int/float literal a
  `ScalarConstantExpression` whose `InlineScalar` is an `InternalValue` built in the node and
  never destroyed, since it owns nothing. `EvaluateRef` and `GetConstant` read it in place, as
  before. `BinaryExpression` owns nothing; the literal `in` list and `%` format moved to two
  subclasses that still own them. A name is read from the tree only on a lookup-cache miss.
  node_arena.cpp checks that `TrivialNodes` lists exactly the classes that own nothing.
  Against P5b-2b: Load -0.6..-5% (expressions -4.97%, chat_llama -2.87%, for_range -2.37%,
  many_tags -2.05%, dict_ops -1.72%, for_filter_if -1.62%), plain_text +0.1%: the set of block
  names is made only past 16 blocks, since even an empty one clears its bucket when destroyed
  (+33 instructions on every parse); Render -2.5..+0.55% (expressions
  -2.52%; large_static +0.55%: its 50 `{{ title }}` nodes each miss the lookup cache once a
  render, and a miss reads the name through the view, three instructions more, while a hit
  reads three fewer; the suite's Render total falls 0.16%). Left for P5b-3b: assignment targets and slot names; P5b-3c:
  blocks, import, with, trans, macro parameters. Attribute names and string literals stay
  owners: a view of them would leak out of a render or cost a string per lookup.
- P5b-3b (assignment targets and slot names in the tree): a `for`/`set` target is an arena
  list of `TargetNode`s in pre-order (names as `ArenaText`, each plain name's place among the
  target's distinct names given by the parser, so binding slots writes nothing back), and a
  slot name is `{ArenaText, hash}` read through the `ArenaView` its `FrameView` carries: the
  loop's or the macro's tree, not the running template's. The relocated hook is gone
  (`NodeOps::relocated`, `OnRelocated`, `ArenaView::Rewrite`); `ForStatement` and the three
  `set` statements own nothing. A parenthesised target is bounded by `MaxExpressionDepth`
  (`RecursionLimitExceeded`; it recursed without a bound before). A `set` inserts by the
  name's stored hash through `try_emplace_transparent`, a local addition to the vendored
  robin_hood.h, so the key string is made only for a new name; the slotted loop filter binds
  a plain name without the unpacking call. Against P5b-3a: Load -0.5..-9.2% (dict_ops -9.22%,
  mitsuhiko_table -6.05%, for_range -5.21%, strings -4.56%, inheritance -3.08%,
  for_filter_if -2.10%), every 0892811 target met (plain_text 4,014, substitute 10,713,
  for_range 19,722, dict_ops 47,800, for_filter_if 41,070, inheritance 35,094,
  for_loop_vars 57,019); Render -2.91..+0.21% (for_filter_if -2.91%, many_tags -0.47%;
  mitsuhiko_table +0.21%, inheritance and macros +0.16%: a frame view is 16 bytes larger).
- P5b-3c (block, import, with, trans and macro names in the tree): a block name is an
  `ArenaText` and `BlocksStack` keys views into the trees its render pins; an import keeps its
  namespace and an `ArenaSpan<ImportName>` (deduplicated at parse: the first place, the last
  alias, as before; past 16 names through a hash map) and finds each name in the imported
  scope; `with` and `trans` keep spans; a macro keeps its name and an
  `ArenaSpan<MacroParam{name, hash, default, refersToArgs}>` that `MakeBinderNames` reuses,
  binds arguments by the stored hash, and works out its declared special names and
  `macro.name`/`macro.arguments` in the constructor, from the parser's strings, so nothing
  reads the texts back at Load. A call block keeps its macro name and `ArenaCallParams`.
  `OrderedMap::find(std::string_view)` compares in place while the map is small and makes
  the key once it is indexed (more than 8 entries), so the index keeps one hash. Blocks,
  imports, `with` and `trans` own nothing; the macro statements still own their attributes.
  Against 50dec81: Load -0.96..0% (macros -0.96%, config_file -0.55%, inheritance -0.29%,
  html_autoescape -0.12%), every 0892811 target met (plain_text 4,014, substitute 10,713,
  for_range 19,699, dict_ops 47,776, for_filter_if 41,013, inheritance 34,964,
  for_loop_vars 56,925); Render -0.18..+0.01% (html_autoescape -0.18%, macros -0.18%,
  config_file -0.08%, inheritance -0.03%); retained memory falls on 4 cases (-64..-264 bytes).

### Resume point (wave 2 paused 2026-10-08, Ruslan)

P5b-3c is in its PR (above). Next, in order: P5c, phase 6 (the lookup-cache rekey, with 0117 P5; send the perf track an
estimate for Render/inheritance, which must reach 202.7k, before writing code), then 0117 P3.
Gate each with `bench/count.py --baseline` against its base and `--cache-sim`: Load ≤ base
+0.5% on every case, Render within ±0.5%, retained memory under the caps.

P5b-3c plan (blocks, import, with, trans, macro parameters; touches statements.*,
template_parser.*, ordered_map.h, render_context.h):
- `BlockStatement`: `ArenaText` name. `BlocksStack` keys become `std::string_view` into
  trees the render pins (document it there); `self` and error texts copy the name.
- `ImportStatement`: `ArenaText` namespace and an `ArenaSpan<{name, alias, hash}>`, deduped
  at parse (first position, last alias wins, as today); `ImportNames` finds each name in the
  imported scope instead of walking the scope.
- `WithStatement`: `ArenaSpan<{ArenaText name, hash, NodeRef<Expression>}>`, assigned in order.
- `TransStatement`: `ArenaSpan<NodeRef<Expression>>` (the names already went into gettext).
- `MacroStatement` (stays an owner for `m_attributes`) and `MacroCallStatement`: name as
  `ArenaText` + hash; parameters as `ArenaSpan<{name, hash, default, refersToArgs}>`, which
  `MakeBinderNames` reuses; keyword lookup through a new `OrderedMap::find(std::string_view)`
  that must hash exactly as its index.
- Expected: Load inheritance -0.4%, macros -0.35%, config_file/html_autoescape -0.1%; Render
  inheritance -0.05..-0.1%, macros ±0.2%. Owners after 3c: `ConstantExpression`,
  `SubscriptExpression`, `LoopAttrExpression`, `InLiteralExpression`, `ConstFormatExpression`,
  `FinalizedExpressionRenderer`, the two macro statements, the filter and tester objects.
- Tests: long block names with `super()`/`self`; long import names and aliases, 40 imported
  names; `with a=1, a=2` gives `2`; `trans` with a long variable and a plural; macro kwargs,
  `caller`, `varargs`/`kwargs`, defaults that use arguments; `OrderedMap` string_view find on
  both paths; `TemplateKeepsOnlyOwners` rows (blocks and `with` 0, a macro 1).
- Divergences seen while building 3b, filed as 0155 (`{% from 'm' import a, a as b %}`) and
  0156 (a call block on an undefined name renders nothing).
