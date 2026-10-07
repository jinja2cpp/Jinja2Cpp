# 0118: give each template's parse tree its own arena (architect plan)

> **Revision 2026-10-06** (after the owner's review and two addenda). The direction is
> unchanged; the access model is new.
> - **Node references.** Raw `NodePtr<T>` is replaced by `NodeRef<T>`: a 32-bit offset
>   that can only be resolved through the template's arena. Builds with checked refs
>   verify bounds, type and arena identity on every access.
> - **Lists and text.** Child lists become `boost::span<const NodeRef<T>>` over arena
>   memory. Raw text becomes a `SourceSpan` into the kept source.
> - **Names (course correction).** Names go into a per-template symbol table inside the
>   arena: `string_view`s into the kept source, each mapped to a 32-bit `SymbolId`,
>   built during the parse and immutable after `Load`, so concurrent renders read it
>   without locks. 0117's slots and the lookup cache key on `SymbolId`. Boost.Flyweight
>   is dropped. A process-wide interner is only a future measurement (section 7).
> - **pmr.** `boost::container::pmr::monotonic_buffer_resource` is now the arena's
>   allocation primitive, running over the template's one contiguous buffer, with
>   `null_memory_resource` upstream. Section 10 explains why it is not a chunked arena.
> - **Escaping objects.** These hold checked handles, never a raw `this`:
>   `{TemplateSlot, NodeRef}` within a render.
> - **Ownership (weak_ptr refinement).** Nothing can keep a template alive forever.
>   - Strong owners of a `TemplateImpl` are only the user's `Template` and the
>     `TemplateEnv` cache.
>   - A render pins its templates once, for its whole duration: the root through the
>     caller's `Template` (no refcount operation), and includes, imports and parents in
>     the per-render pin set (`Loaded()`). With `EveryUse`, a freshly loaded include or
>     import is owned by the importing render's pin set, for that render only.
>   - Inside a render: plain Debug-checked `NodeRef` and slot handles, no refcounts.
>   - Anything that can outlive a render holds `weak_ptr<const TemplateImpl>` plus a
>     `NodeRef`, locked on use. That covers macro, `caller` and `loop()` callables
>     handed to user callables or stored across renders, and imported module objects
>     kept in values. A dead template gives the new `ErrorCode::TemplateExpired`
>     instead of UB.
>   - No benchmark case ever takes the locked path.
> - **No more nodes made during a render.** Option A's local arenas are gone.
> - **Prototype.** `NodeRef` was prototyped and measured on the renderer family
>   (section 4A).
> - **Libraries.** `boost::span` replaces span-lite as the default. New section 9 covers
>   ranges in the engine, section 10 the other Boost libraries.
> - **Remaining raw pointers.** All listed and justified (section 5.1).
> - **Already done since the first version.** P0 landed as 0134 (#410/#411: a failed
>   reload keeps the previous template, and the fuzz harness reloads every input), so
>   decision 1 is settled. Line references are refreshed to master 62dda01.
>   Measurements are from 448bb7c; 0132-0134 changed render paths, not Load.

> **Revision 2 2026-10-06** (the owner's 07:51 decision, plus the cache-locality note).
> - **`NodeRef` checks are on in every configuration, Release included.** CMake option
>   `JINJA2CPP_NODEREF_CHECKS` (`ON` by default, also `OFF` and `FULL`) maps to a
>   define. Decision 4 is rewritten.
> - **Measured:** a per-access range check costs too much to run on every link, so
>   `ON` verifies all stored refs once at `Seal()` and checks per access only the
>   handles that come from outside the arena (section 4A).
> - **Always on:** `weak_ptr` expiry (`TemplateExpired`) stays regardless of the
>   switch.
> - **P3 gains two layout requirements** from perf-cache-locality-notes.md: a hot/cold
>   split (cold fields such as source positions in a side table keyed by `NodeRef`;
>   hot nodes 16-32 B) and child spans placed right after their parent, with
>   statements in pre-order.

> **Revision 3 2026-10-06** (the integrated cache review). Read
> `perf-design-overview.md` first: it supersedes this plan's phase order and footprint
> target where they differ.
> - **Kept:** virtual nodes in the arena, 32-bit `NodeRef`, symbol table, weak escapes.
> - **P3 adds:**
>   - a 1-byte node kind;
>   - fixed-arity children as inline refs (spans only for variable lists);
>   - source positions only for nodes that can fail at render.
> - **Retained target for `many_tags`:** 0.20-0.25 MB, down from 0.35-0.45 MB.
> - **Lookup cache:** keyed by `{TemplateSlot, SymbolId}` with 32 entries, measured.
> - **Gates:** cachegrind D1/I1 columns (new task N1) on P3-P5.
> - **Bytecode:** stays out, and is not the answer to the Load gap against MiniJinja.
>   That gap is per-tag work: the allocator here, then a single-pass scanner (N6).
> - **Check mode still open:** `NodeRef` validation at `Seal()` versus on every access
>   is still with the owner. Revision 2's text describes the `Seal()` option, and the
>   overview's section 3 gives the costs of both.

> **Revision 4 2026-10-07** (phase P4c, perf-track/0118-p4c-plan.md). `FULL` keeps the
> 4-byte layout: no `{offset, arena id, type tag}` refs, no object header, no
> `dynamic_cast`. The node's 1-byte kind is the type tag. `Seal()` checks every link in
> `ON` and `FULL`: range, alignment, that it lands on a node's start, and the kind family.
> Links into another template's tree are checked where they are followed
> (`TemplateSlots::Resolve`). `FULL` adds the same checks on every access, plus a mark of
> the nodes `Seal()` reached, so a link a node's `VisitRefs` forgets fails at first use.
> Measured: about 20 instructions per link at `Seal()`, not 3; Load many_tags +2.66%
> against P4c-1, Render unchanged. The `FULL` row of the table in section 4A is superseded.

## 0. Summary

**Ownership is already per template.**
- Each node is held by a `shared_ptr`, but every path by which a node outlives its
  parent node or its render already uses a raw pointer. Those paths are safe only
  because something holds a `shared_ptr<TemplateImpl>`, or because they never leave
  the render.
- So the per-node `shared_ptr` buys nothing. Lifetime is per `TemplateImpl` today, and
  the arena makes that explicit.

**Recommendation.**
- One owner: each `TemplateImpl` owns one contiguous arena, bump-allocated through
  `boost::container::pmr::monotonic_buffer_resource` with `null_memory_resource`
  upstream. The arena holds the nodes, the child lists, the filter objects and the
  symbol table. A constants table in the same `TemplateImpl` holds the constants.
- Access only through checked handles: `NodeRef<T>`, `ArenaSpan<T>`, `SymbolId`,
  `ConstRef`, `SourceSpan`, all resolved through the arena or the render context.
- `NodeRef` cost, measured on the renderer family (child lists resolved through a base
  held in the render context): Render +0.11% to +0.42% instructions, wall clock within
  noise. Estimate if every child link goes through the base: about +0.5-1% on
  `mitsuhiko_table` and +1-2% on `expressions`.
- Memory: a 4-byte ref against a 16-byte `shared_ptr` saves 12 B per link (about
  140 KB on `many_tags`). Spans, `SymbolId`s and the per-node overheads the arena
  removes add more; the combined estimate is `many_tags` retained 1.06 MB → about
  0.35-0.45 MB.

**Ownership.**
- Strong references to a `TemplateImpl` exist only in the user's `Template`, the
  environment cache and the render's pin set.
- Objects that can outlive a render hold `weak_ptr` plus a `NodeRef`, and fail with
  `TemplateExpired` once the template is gone.

**The cheap wins still come first.** The 0130 and 0131 items that do not need the arena
still land first.

## 1. Current state (master 62dda01)

### 1.1 How the tree is owned
- Every node is a separate heap object reached through `std::shared_ptr`, made with
  `std::make_shared`:
  - `RendererPtr` (src/renderer.h:35);
  - `ExpressionEvaluatorPtr<T>` (src/expression_evaluator.h:52-53);
  - `StatementPtr<T>` (src/statements.h:31-32).
  - There are 225 uses of these aliases in 11 files, and about 80 `make_shared` sites
    for nodes: src/expression_parser.cpp (36), src/template_parser.cpp (29),
    src/template_parser.h (6), plus filters, testers and `function_base.h`.
- Bodies are `small_vector<RendererPtr, 4>` (renderer.h:90). The root grows on the
  heap.
- Expression children are `shared_ptr` fields, and so are filters and testers
  (expression_evaluator.h:901-904, :646).
- Names are `std::string`:
  - `ValueRefExpression::m_valueName` (:361);
  - `SubscriptExpression::Index::attrName` (:421);
  - `AssignTarget` (statements.h:66-73);
  - `MacroParam::paramName`, `BlockStatement::m_name`, and others.
- Constants are `InternalValue` (72 B; :496).
- `ExpressionRenderer` keeps a 72-byte copy of `Settings::finalize` (renderer.h:174).
- Every renderer carries two extra vptrs:
  - `IRendererBase : public virtual IComparable` (renderer.h:23);
  - `VisitableStatement` (ast_visitor.h:25), whose visitor is not used anywhere.
- Text: `RawTextRenderer` points into the source, which `TemplateImpl` keeps since 0134
  as `unique_ptr<std::basic_string>` (renderer.h:98-135, template_impl.h:614). A copy
  converted for `newline_sequence` is held by `shared_ptr<const void>`
  (template_parser.h:1244-1254).
- Filters: each instance derives from `FunctionBase`, which holds a
  `ParsedArgumentsInfo` (272 B) and `std::string m_argsError` (function_base.h:66-67).
- Static default nodes are shared by every template (`MakeArgumentsTable`,
  function_base.h:27; used by `SetDefaultArg`, expression_evaluator.cpp:1100).

### 1.2 How Template, TemplateEnv and the cache share a loaded template
- `BasicTemplate` holds `std::shared_ptr<ITemplateImpl> m_impl` (include/jinja2cpp/template.h:196).
  Copies share it; the tree is never copied.
- `TemplateImpl::Load` parses into fresh state and commits the source, tree and name
  together only on success (template_impl.h:189-210, since 0134).
- The environment cache stores `Template` values (template_env_impl.h:46-77;
  template_env.cpp:118-191).

### 1.3 How a render keeps other templates alive
- `RendererCallback::Loaded()` keeps every `TemplateImpl` the render resolved until the
  render ends, including templates reloaded under `EveryUse` (template_impl.h:484-605).
- `ParentTemplateRenderer` (statements.cpp:1108), `IncludedTemplateRenderer` (:1153)
  and `ImportedMacroRenderer::m_module` (:1289, :1350) hold `shared_ptr<TemplateImpl>`.
- `BlocksStack` holds raw `const BlockStatement*`, and keeps their templates alive
  through `parents` (statements.h:495-500).

### 1.4 Paths by which a tree object can outlive its parent node or the render, and what each becomes
| What | Where (62dda01) | Holds today | Revised | Strictly safer? |
|---|---|---|---|---|
| macro callable | statements.cpp:1441 | raw `this` | `{TemplateSlot, NodeRef<MacroStatement>}` | yes: a use outside its render is a checked error, not a use-after-free |
| `caller` | statements.cpp:1707 | raw `this` | same | yes |
| `loop(...)` | statements.cpp:601 (made at :443) | raw `ForStatement*` | `{TemplateSlot, NodeRef<ForStatement>}` + level (12 B, fits `std::function`'s small buffer) | yes |
| `super()` | statements.cpp:972, 979 | raw `this`, `BlocksStack*` | `{TemplateSlot, NodeRef<BlockStatement>}` + depth; the stack is reached through the template frame of the context | yes for the node; the stack was already render-scoped |
| block stack entries | statements.h:497 | `const BlockStatement*` + `parents` keep-alive | `{TemplateSlot, NodeRef<BlockStatement>}`; parents are registered in the render's template table | yes |
| imported macros | statements.cpp:1289-1350 | a chain `m_module` → `IncludedTemplateRenderer` → `shared_ptr<TemplateImpl>` | inside the render: the module holds a `TemplateSlot` into the importing render's pin set, and its macros hold slot handles. When the module or a macro is converted to a value that can leave the render: `weak_ptr<const TemplateImpl>` + `NodeRef` | yes: no value holds a template strongly any more |
| `self.<block>` | statements.cpp:1032 | the name, by value | unchanged | — |
| lazy filtered loop list | statements.cpp:737 | raw `this` + `RenderContext&` | `NodeRef<ForStatement>` + `RenderContext&` (kept, see 5.1) | partly |
| filters and testers made during a render | filters.cpp:732, 1227 | copies of node `shared_ptr`s | `NodeRef`s into the calling template's arena; used only inside the `Filter()` call that made them | yes |
| nodes made during a render | global_functions.cpp:438-444 (gettext `_` builds a `CallExpression`); `SetDefaultArg` at render (expression_evaluator.cpp:948, 1100); filters.cpp:692 when `Map` is made at render | owning `shared_ptr` | **removed**: `_` calls the gettext callable directly; defaults become `ArgSource{Default, index into the static table}`; the `'attr'` literal becomes `ArgSource{Literal, …}` | no node is made during a render |
| name lookup cache | render_context.h:86-125; `Forget(this)` at expression_evaluator.h:343 | the node's address as the key | key `{TemplateSlot, SymbolId}` packed in 64 bits, so every use of a name in an epoch shares one entry; `Forget` goes away (no nodes are made during a render) | yes |

Values reach user code only as arguments to user callables:
- callables convert to an empty `Value` (internal_value.cpp:1692);
- lists and maps become lazy wrappers (:1669);
- the `context` argument wraps a raw `RenderContext*` (:1818).

The render result is a string. **No node `shared_ptr` is ever copied into an object
that outlives the `TemplateImpl`.** Python allows both escapes tested in section 6
(`loop` and a macro kept in a namespace past their scope); Jinja2C++ renders them the
same way, memcheck-clean.

### 1.5 Concurrency
- Rendering only reads the tree (`TemplateApiTest.ConcurrentRenderOfOneTemplate`,
  test/template_api_test.cpp:142).
- 0133 added `ForStatement::m_loopId` (statements.h), a process-unique id used instead
  of the node's address. It stays a plain field.

### 1.6 Reload bug: fixed in 0134
- 0134 parses into a heap-allocated source and commits on success. It also added a
  reload step to the fuzz harness.
- Side effect: one extra allocation per `Load`, for the `unique_ptr<std::string>`. The
  arena removes it by storing the source as the arena's first object, or as a side
  buffer of the same owner.

## 2. Where Load time and bytes go today

Measured with `bench/count.py`, master 448bb7c (Load paths are unchanged since):

| Benchmark | Instructions | Allocations | Bytes requested | Retained | Peak |
|---|---:|---:|---:|---:|---:|
| Load/plain_text (25 B) | 8,840 | 15 | 1,626 | 1,120 | 1,424 |
| Load/substitute | 16,943 | 26 | 2,771 | 1,520 | 2,344 |
| Load/mitsuhiko_table (623 B) | 98,333 | 94 | 17,880 | 6,064 | 12,072 |
| Load/chat_llama (3.5 KB) | 668,397 | 402 | 69,850 | 39,416 | 53,112 |
| Load/large_static (37 KB) | 300,435 | 192 | 75,919 | 53,352 | 63,224 |
| Load/many_tags (39 KB, 300 lines, 2,400 tags) | 16,456,836 | 9,658 | 1,597,951 | 1,060,056 | 1,271,088 |

The 0118 goal is -30% against 0109's 16.40M, that is **≤ 11.48M**.

**Callgrind, Load/many_tags:**
- malloc and free internals: about 21% self (`_int_malloc` 7.3, `_int_free` 4.0,
  `malloc` 2.6, `malloc_consolidate` 2.5, `free` 1.6, `unlink_chunk` 1.0).
- Destroying the template: 9.6% inclusive, through cascading `shared_ptr` releases.
- `ExpressionFilter` construction: 5.7%.
- The lexer: about 11%.
- `expected<shared_ptr<...>>` handling: about 3.3%. With a trivially copyable 4-byte
  ref it mostly disappears.

**Load/plain_text** is fixed cost: `MakeDelimiters` 31%, the `Settings` copy 23%,
destruction 12.6%. The arena does not help it (0131).

**Node sizes** (GCC 13 x86-64):

| Type | sizeof (B) |
|---|---:|
| ComposedRenderer | 104 |
| ExpressionRenderer | 104 |
| SubscriptExpression | 112 |
| BinaryExpression | 104 |
| CallExpression | 136 |
| SetLineStatement | 128 |
| ConstantExpression | 80 |
| ExpressionFilter | 72 |
| IfStatement | 88 |
| RawTextRenderer | 56 |
| ValueRefExpression | 48 |
| FilteredExpression | 40 |
| filters::StringConverter | 320 |
| filters::Default | 312 |
| filters::Map | 424 |

**Where 3.5 KB per `many_tags` line goes, and what each change removes:**

| Item | Bytes per line today | After the change | Saved on many_tags |
|---|---:|---|---:|
| ~28 control blocks + malloc headers and rounding | ~900 | 0 (arena) | ~270 KB |
| ~40 child links as `shared_ptr` (16 B) | 640 | 160 as `NodeRef` (4 B); raw pointers would be 320 | ~144 KB (another 48 KB beyond raw pointers) |
| 2-3 bodies as `small_vector<RendererPtr,4>` (~88 B inline) | ~220 | 8 B `ArenaSpan` + 4 B per child | ~50 KB |
| ~10 names as `std::string` (32 B, all SSO; 3,600 name tokens, 307 distinct) | 320 | 40 as 4-byte `SymbolId`s; the symbol table is 307 × 16 B ≈ 5 KB for the whole template (views into the source, so no characters are copied) | ~85 KB |
| 2 filter objects (`FunctionBase`) | ~700 | ~150 compacted (P2a) | ~165 KB |
| 4 `finalize` copies | 288 | 0 (P2b) | ~86 KB |
| 2 extra vptrs on ~11 renderers | ~180 | 0 (P2b, decision 5) | ~50 KB |

The estimates overlap (filter objects also contain links and names). The combined
estimate is **0.35-0.45 MB retained**, against 0130's goal of under 0.53 MB.

## 3. The conflict, and its overlap with 0130 and 0131

**Requirement 1, compact and cheap:**
- one allocation per template;
- free destruction;
- 4-byte links;
- nodes laid out in tree order.

**Requirement 2, memory-safe:**
- one owner;
- no raw pointers to nodes;
- copies and the cache share one tree;
- many renders at once without locks or refcount traffic;
- a render may load or reload other templates.

**How they meet:**
- The owner is `TemplateImpl`. Its lifetime is already the lifetime of every escape in
  1.4.
- Inside a template, all access goes through `NodeRef`s resolved against that
  template's arena, checked in checked builds.
- A reference that crosses a template boundary carries a `TemplateSlot`: an index into
  the render's table of the templates it keeps alive (the root plus `Loaded()`),
  checked on every use.
**Who holds a template strongly (the weak_ptr refinement).**
- Owners: the user's `Template` (`BasicTemplate::m_impl`, the public
  `shared_ptr<ITemplateImpl>`) and the environment cache.
- Pins: a render pins every template it uses, once, for its whole duration.
  - The root is pinned by the caller's `Template`, which must outlive the `Render`
    call it makes. Taking a copy would cost two contended atomics per render on
    concurrent renders of one template (decision 16).
  - Includes, imports and extends parents are pinned by the `shared_ptr` that
    `Loaded()` already keeps per render (template_impl.h:484-605). Under `EveryUse`, a
    template reloaded during the render belongs to that render's pin set and dies with
    it, unless a cache keeps it.
  - `ParentTemplateRenderer`, `IncludedTemplateRenderer`, `ImportedMacroRenderer` and
    `BlocksStack::parents` stop holding `shared_ptr`s. They hold `TemplateSlot`s into
    the pin set.
- Escapes: an object that can leave the render holds `weak_ptr<const TemplateImpl>`
  plus a `NodeRef`. That means a callable converted to a value for a user callable or
  stored across renders, or an imported module object kept in a value. It calls
  `lock()` on each use.
  - The `weak_ptr` comes from `weak_from_this()` (C++17; `TemplateImpl` derives from
    `enable_shared_from_this`, which works with the `make_shared` in template.cpp:57).
  - `lock()` failing raises `ErrorCode::TemplateExpired`, a new value 13 in
    include/jinja2cpp/error_info.h. `ExtraParams[0]` is the template name and `[1]` the
    macro name; the message goes in src/error_info.cpp.
  - Adding an enumerator is source- and ABI-compatible. Users who switch on
    `ErrorCode` with `-Wswitch` see one new case.
  - When an expired callable runs inside a later render, that render returns the
    error. A user callable that invokes it outside any render has no context to run
    in, which is already the case for every macro; it gets an empty `Value`.
- **Cycles.**
  - Today, no template-to-template cycle is possible: tree nodes hold no template
    references, and render-time holders die with the render.
  - The first version of this plan (an owning handle for escaping objects) would have
    made one possible: a macro callable stored by a user callable into
    `TemplateEnv::AddGlobal` would hold its template, the template holds the
    environment (`m_envHandle`), and the environment holds the global. `~TemplateEnv`
    drops caches, not globals, so that cycle would never be freed.
  - With `weak_ptr` at every escape, no value holds a template strongly, so the cycle
    is impossible.
  - The existing environment ↔ cache loop (template_env_impl.h:20-22) is unchanged
    and still broken by `~TemplateEnv`, as today (decision 17).
- The render path takes no atomic operation: no pin copy for the root, no refcount
  inside the render. `lock()` runs only on calls through escaped callables.
- Cost of `lock()`: an atomic load plus a compare-and-swap, and a decrement on
  release, about 10-20 ns uncontended. A macro call costs several hundred
  instructions, so an escaped call pays under 5% more.
- No benchmark case pays it: the bench drivers register no user callables or globals
  (no `Callable` or `AddGlobal` in bench/*.cpp), so no case can call an escaped
  callable, in a loop or otherwise. Creating the `weak_ptr` (one weak-count increment)
  happens only when a callable is converted for a user callable, which no case does
  either.

**0130 and 0131 items that do not need the arena** (land first; unchanged from the
first version):

| Item | Task | Expected effect | Touches |
|---|---|---|---|
| Per-environment `MakeDelimiters` and keyword tables; share `Settings` instead of copying | 0131 | plain_text 8.8k → ≤4.4k instructions, 15 → ≤8 allocations | template_impl.h, template_parser.h/.cpp |
| `ExpressionRenderer` without `m_finalize` | 0130 | -86 KB on many_tags | renderer.h, expression_parser.cpp |
| Compact `FunctionBase`: arguments as `ArgSource` (8 B: {Node, Default, Literal, Missing} + index) in an exact-size array; the error as an error code or `unique_ptr<std::string>` | 0130 | -100..-165 KB; also removes the default nodes made during a render (1.4) | function_base.h, filters.*, testers.* |
| Remove the unused `VisitableStatement` | 0130 | -8 B per renderer | ast_visitor.h, renderer.h, statements.h |
| Shrink the root body after parsing | 0130 | ~13 KB | template_parser.h |

## 4. Options

### Option A: one contiguous arena per template, checked 32-bit `NodeRef`s (recommended)

**The arena (`src/node_arena.h`).**
- One `std::unique_ptr<std::byte[]>` per `TemplateImpl`. A
  `boost::container::pmr::monotonic_buffer_resource(buffer, size,
  null_memory_resource())` does the bump allocation over it. When the buffer is full,
  the null upstream throws `bad_alloc`, which `Load` catches to re-parse with twice the
  buffer. The buffer is sized from the rough parse:
  `DoRoughParsing` already knows the number of blocks and the characters inside tags
  before any node exists. The coefficients are calibrated on the bench and parity
  corpus.
- If the estimate is too small, double the size and parse again. Parsing is
  deterministic, so this costs one extra parse, only for misestimated templates.
  Calibrate for a re-parse rate under 1% and slack under 25%.
- Contiguity is what lets one base pointer plus a 32-bit offset work.
- Alternatives if re-parsing turns out costly (decision 8):
  - a chunk table: `chunks[off >> 26] + (off & mask)`, one more load per access;
  - relocating the buffer, legal only if every arena object is trivially copyable,
    which needs non-virtual dispatch (section 7).
- `Make<T>(args...)` returns a `NodeRef<T>`. If `T` is not trivially destructible, it
  records `{dtor-thunk, offset}` in a cleanup vector owned by the arena. Recording
  happens through a `static_assert`ed trait, so it cannot be forgotten.
- `Seal()` after `Load`: in checked builds, `Make` asserts once sealed.
- The arena never allocates during a render.
- No pmr containers in retained nodes: they store absolute pointers plus a resource
  pointer. Retained lists are `ArenaSpan`s. Parse-time scratch (vectors that get
  copied into spans, the symbol interning map) stays out of the template's buffer, so
  it leaves no slack there.

**`NodeRef<T>`.**
- Release: `uint32_t` offset, trivially copyable, 4 B.
- **Checks: CMake option `JINJA2CPP_NODEREF_CHECKS`.** It is set per target, never
  through `NDEBUG`, so translation units cannot disagree on the layout.

  | Mode | When | What it checks | Ref size |
  |---|---|---|---|
  | `ON` | default in Release, RelWithDebInfo and MinSizeRel builds | at `Seal()`, one pass over every `NodeRef` and `ArenaSpan` stored in the arena: `offset + size <= used`, and a 1-byte node kind matches the expected family. The sealed arena is immutable afterwards, so refs read from it cannot change. Per access: range and arena generation on handles from outside the arena (`{TemplateSlot, generation, NodeRef}` render-scoped handles, escape handles). Failure throws an internal error, which the render reports as `UnexpectedException` with "invalid node reference". | 4 B in nodes; handles carry a 16-bit generation |
  | `FULL` | default in Debug, ASan and fuzz builds | everything in `ON`, plus range, alignment, kind and reached-by-`Seal()` checks on every access (Revision 4; the 8-byte layout this row first proposed was dropped) | 4 B |
  | `OFF` | opt-in, for embedders who want the last percent | nothing except what is always on (next point). It removes the `Seal()` verification pass, the per-access range checks on handles, and the arena-generation compares on handles | 4 B |

- **Always on, whatever the switch:**
  - the `weak_ptr` expiry check on escaping objects (`lock()` → `TemplateExpired`);
  - the `TemplateSlot` index bound when a slot is resolved (part of resolving it).
- **Why `ON` is not per access** (measured on the prototype, body children only;
  section 4A table):
  - The straightforward per-access check costs +2.27% on mitsuhiko_table on top of the
    indirection.
  - A tight 32-bit compare against a limit stored next to the base costs
    +1.36% (mitsuhiko_table), +0.43% (expressions), +0.58% (many_tags), +0.56%
    (macros), +1.26% (large_static). That is about 2 instructions per resolve.
  - Extended to every link, that is about +2.5-3% on mitsuhiko_table and +2-3% on
    expressions, on top of the indirection. That exceeds the budget of +0.5-1%
    (mitsuhiko_table) and +1-2% (expressions), so per-access checks on every link are
    `FULL` only.
  - The `Seal()` pass costs about 3 instructions per stored ref (about 12k refs on
    `many_tags`, about +0.2-0.5% Load) and nothing at render.
- Dereferencing: `arena[ref]` / `ref.get(arena)` returns `T&` through
  `std::launder(reinterpret_cast<T*>(base + offset))` (the object was
  placement-constructed there).
- At render, `ctx.Nodes()` is the current template's `ArenaView`.
- No implicit conversion to a base class. `arena.As<Base>(ref)` converts through real
  pointers at parse time, so multiple or virtual inheritance cannot skew offsets.
  Decision 5 (dropping the virtual `IComparable` and the visitor) leaves single
  inheritance everywhere.

**Lists, names, text and constants.**
- `ArenaSpan<T>`: `{uint32 offset, uint32 count}`, viewed as
  `boost::span<const NodeRef<T>>` through the arena. Parsers fill scratch vectors that
  are reused across tags, then copy them once at the end of the block.
- **The per-template symbol table** (`SymbolTable`, in the arena).
  - Retained part: an array of `Symbol{SourceSpan or pool offset, uint32 length,
    uint64 hash}`, 16 B each and trivially destructible, indexed by a 32-bit
    `SymbolId`. Nodes store `SymbolId` (4 B) wherever they stored a `std::string` name.
  - Building it: during the parse, a transient `name → SymbolId` hash map (parse-time
    scratch, not retained) interns each identifier the first time it appears: 307
    symbols for `many_tags`'s 3,600 names. After `Seal()` the table is immutable, so
    concurrent renders read it without locks or refcounts.
  - Narrow templates: the name is a `string_view` into the kept source.
  - Names that are not in the source go into a narrow character pool in the arena:
    converted names of wide templates, and synthesized ones (`caller`, `varargs`,
    `kwargs`, `loop`, `$transN`).
  - Precomputed hash: `HashedName` (internal_value.h:724) is built from the symbol
    without hashing at render, as `ValueRefExpression` does today with `m_nameHash`.
  - The one exception to "views only": `IMapItemAccessor::GetValueByName(const
    std::string&)` (public) and `ValuesMap` lookups. `std::unordered_map<std::string,
    Value>` has no heterogeneous lookup before C++20, so it needs a `std::string`
    key. Symbols used as attribute names (`x.name`) therefore also get a `std::string`
    mirror, built at `Load` in a vector owned by the symbol table. That is 2 strings
    for `many_tags`, and attribute access builds no string per render, as today.
  - Consumers: 0117's name slots resolve `SymbolId → (depth, slot)` at `Load`, and the
    lookup cache keys on `{TemplateSlot, SymbolId}`.
- `SourceSpan` `{uint32 offset, uint32 length}` into the kept source, for raw text.
  Converted text goes into an arena character pool.
- `ConstRef` (4 B): an index into a per-template `std::vector<InternalValue>`. With
  it, `ConstantExpression` is trivially destructible.

**How a render finds the arena.**
- `RenderContext` holds `ArenaView{base, size, id}` for the current template.
- It is switched by RAII guards on every crossing between templates: include, the
  parent of an extends, an imported macro or `caller` invoked through its slot handle,
  and a block rendered from a parent (`RenderBlockAt`, `super`, `self`).
- If a guard is missing, every access from the wrong template fails the arena
  identity check in checked builds. Every such crossing is exercised by existing
  tests (section 6).

**Cost of the indirection: prototype and measurement.**
- Setup: a scratch copy of b7e7b2b, outside the repo checkout, not committed.
  `ComposedRenderer` stores its children as `int32` offsets from a base held in
  `RenderContext` and set in `TemplateImpl::Render`, and resolves them at render
  (`base + off`). The `shared_ptr` vector was kept for ownership, so the Load row
  shows double bookkeeping, not the design.
- Instruction counts (`count.py`):

| Benchmark | Baseline | NodeRef bodies | Change |
|---|---:|---:|---:|
| Render/mitsuhiko_table | 7,263,483 | 7,285,561 | +0.30% |
| Render/expressions | 555,195 | 555,814 | +0.11% |
| Render/many_tags | 1,293,871 | 1,295,697 | +0.14% |
| Render/macros | 1,179,828 | 1,183,647 | +0.32% |
| Render/large_static | 31,946 | 32,079 | +0.42% |
| Render/for_loop_vars, for_range, chat_qwen | | | +0.15..+0.27% |
| Load/many_tags | 16,460,879 | 16,634,382 | +1.05% (prototype keeps both vectors) |

- Wall clock: three interleaved runs of five repetitions. Medians for mitsuhiko_table:
  base 659/731/773 µs against 702/648/682 µs; for expressions: 56/50/48 against
  53/55/52 µs. Within noise.
- Per access: about 0.5 instructions (46k body-child accesses per mitsuhiko render).
  Field links in expression nodes cannot reuse a base hoisted across a loop: about one
  extra load per non-leaf visit plus one add per child.
- Estimate for every link: **+0.5-1% on `mitsuhiko_table`, +1-2% on `expressions`.**
  This is the Release cost with `JINJA2CPP_NODEREF_CHECKS=ON`, within the owner's
  budget.
- Per-access check variants (same scratch build, `count.py`, change against the same
  baseline):

| Benchmark | Indirection only | + check, 64-bit arithmetic, two context loads | + check, 32-bit compare against a limit next to the base |
|---|---:|---:|---:|
| Render/mitsuhiko_table | +0.30% | +2.58% | +1.67% |
| Render/expressions | +0.11% | +0.83% | +0.54% |
| Render/many_tags | +0.14% | +1.05% | +0.72% |
| Render/macros | +0.32% | +1.26% | +0.88% |
| Render/large_static | +0.42% | +2.32% | +1.68% |
| Render/for_loop_vars | +0.27% | +1.00% | +0.58% |
- The fallback, if `expressions` regresses by more than 2% (decision 9):
  self-relative refs (`target = owner + off`, resolved through the owning node).
  These need no base load and cost what a pointer costs, plus an add. They need the
  contiguous arena, and can still check the arena identity through a header.

**Gains:**
- many_tags allocations 9,658 → about 2,000: what remains is the lexer and parser
  buffers, the side tables and the non-trivial filter members.
- Instructions -18..-22% from P4, about -30% with P5 and the 0130 items.
- Retained 0.35-0.45 MB.

**Risks:**
- A missing arena-switch guard. Covered by the identity checks and the tests.
- Misestimated sizes (re-parse rate, slack).
- Node destructors must never touch other nodes.

**Size:** about 1,500 changed lines over P3-P5. Nothing in `include/` changes; the ABI
is unaffected.

### Option B: `shared_ptr` allocated from a pmr arena (`boost::container::pmr::monotonic_buffer_resource` + `allocate_shared`)
- Keeps 16-byte links, control blocks (now in the arena), cascading destruction, and
  atomic refcounts.
- For safety the allocator must hold a strong reference to the arena: 16 B and an
  atomic increment per node.
- About -12% instructions and -10% bytes, for as much work as A. Not memory-safer than
  A, and does not meet the "checked handles" principle. Rejected (pmr in detail in
  section 10).

### Option C: no arena; shrink and pool
- The section 3 items plus `string_view` names. Intrusive refcounts would have to be
  atomic, because render threads copy node pointers when they instantiate runtime
  filters.
- About -8% instructions, -35% retained. It is the "first" half of A, and misses the
  0118 goal on its own.

## 5. Recommendation and phases (one PR each)

Each phase is measured with `bench/count.py --baseline` on Load/many_tags,
Load/plain_text and Load/mitsuhiko_table. Render/* must stay within +1%, except P4 and
P5, which may spend up to +2% on Render/expressions for the `NodeRef` indirection (to
be confirmed with the owner). P4 and P5 also need `run.py` and `--threads` MT/Render.

| Phase | Task | Change | Expected (many_tags unless noted) |
|---|---|---|---|
| ~~P0~~ | 0134 | done (#410/#411) | — |
| **P1** | 0131 | per-environment delimiters and keyword tables, shared `Settings`, no extra copies of the name and source | plain_text ≤4.4k instructions, ≤8 allocations |
| **P2a** | 0130 | compact `FunctionBase` with `ArgSource`; no default nodes made during a render; gettext `_` without nodes | retained -100..-165 KB; instructions -2..-3% |
| **P2b** | 0130 | `finalize` out of `ExpressionRenderer`; drop the visitor vptr; shrink the root; decision 5 (equality, virtual `IComparable`) as its own PR | retained -110..-150 KB |
| **P3** | 0118 | **layout requirements** (perf-cache-locality-notes.md, items 1-2): (a) **hot/cold split**: a node's hot part is the vptr, a 1-byte kind and flags, and its child refs or span; target 16-32 B (for example `BinaryExpression` 24, `ValueRefExpression` 16 with the hash in the symbol table, `ExpressionRenderer` 16, `ConstantExpression` 16 with `ConstRef`). Cold fields go to side tables: source positions as `(NodeRef offset, line, col)` appended in allocation order, so already sorted and found by binary search on error, costing no hot bytes (today render errors report 1:1, template_impl.h:283-320, so this also enables real locations later); `BinaryExpression`'s constant items and percent format go into the constant table. (b) **Pre-order placement**: a statement's child span is reserved right after the statement node, using direct-child counts from a pass over the rough parse's block list, then filled as children are parsed; if a count disagrees (error paths), the span goes at block end, which affects locality only. Statements come out in pre-order. Expression subtrees stay post-order, which matches the operand-first evaluation. Then: introduce `NodeRef<T>`, `ArenaSpan`, `SymbolId`, `ConstRef` and the arena accessor API behind a factory still backed by `shared_ptr` (refs wrap a pointer in this phase); parsers, `CreateFilter` and `CreateTester` take the factory; nodes read children only through `ctx.Nodes()` or `arena[...]` | ±0.5% everywhere (proves the API change is neutral) |
| **P4** | 0118 | the contiguous `NodeArena` (pmr monotonic resource over one buffer) with offset refs; `TemplateSlot` table over the pin set and arena-switch guards; escape handles from 1.4 (slot handles in the render, `weak_ptr` + `NodeRef` on escape, `TemplateExpired`); filter objects in the arena with cleanup; `JINJA2CPP_NODEREF_CHECKS` (`ON` default; `FULL` in Debug, ASan and fuzz) | allocations ≤2,500; instructions -18..-22% against P3; retained ≤0.55 MB; Render +0.5..+2% |
| **P5** | 0118 | nodes trivially destructible: `ArenaSpan` bodies, the `SymbolId` symbol table, `ConstRef`, `SourceSpan` text; lookup cache keyed by `{TemplateSlot, SymbolId}` | ≤11.48M (-30% against 16.40M); retained 0.35-0.45 MB; destruction ≈ one `delete[]` plus side tables |

**Order:**
- P1 and P2a/b can run in parallel threads.
- P3 conflicts with most parser tasks (0098, 0117, 0091, 0127, 0072). One thread owns
  expression_evaluator.h, statements.h and template_parser.* from P3 to P5.
- 0117 (name slots) builds on P5's `SymbolId`: slots resolve `SymbolId → (depth, slot)`
  at `Load`.

### 5.1 Raw pointers that remain, and why
| Where | Why it stays |
|---|---|
| Inside `NodeArena`: the buffer base and `ArenaView::base` | the allocator itself; owned by `unique_ptr<std::byte[]>`; never handed out except as a `T&` from `arena[ref]` |
| `T&` / `const InternalValue*` returned by accessors (`arena[ref]`, `EvaluateRef`, `GetConstant`) | short-lived borrows, valid until the next evaluation (the documented contract of `EvaluateRef`, expression_evaluator.h:39-42); not stored |
| `RenderContext&` / `OutStream&` parameters; `RenderContext::m_parent`, `m_currentScope`, `m_templateFrame`, `ScopeRef` (render_context.h) | stack-disciplined render state nested inside one render; replacing them with handles would cost the hottest path (name lookup); they hold no tree nodes |
| `LookupCache::Entry::slot` (render_context.h:95-100) | a pointer to a scope slot in node-stable maps, guarded by the epoch; its key becomes `{TemplateSlot, SymbolId}` instead of an address; 0117 later replaces most scope maps with slot arrays |
| `RenderContext&` captured by the lazy filtered loop list (statements.cpp:737) | the list is consumed inside the loop's render, and its predicate must run in the loop's scope; it is render-scoped like `context` |
| `ContextMapper(&context)` given to user callables (internal_value.cpp:1818), and the public accessor interfaces returning `const IMapItemAccessor*` | public API, contract "valid during the call" (decision 2) |
| `TemplateImpl::m_env` (alias of `m_envHandle`), `RendererCallback::m_host` | non-owning aliases of an owner in the same object or stack frame; can become references. `m_host` is also how the root is pinned: the caller's `Template` owns it for the duration of `Render` |
| `shared_ptr<TemplateImpl>` (not a raw pointer, listed for completeness) | only in `BasicTemplate::m_impl` (public), the environment cache, and the render's pin set (`Loaded()`). Everything else holds a `TemplateSlot` (inside a render) or a `weak_ptr` (escapes) |
| C API | none exists (no `extern "C"` in include/ or src/) |

### 5.2 Span library
**Default: `boost::span` (boost/core/span.hpp)**, in the pinned Boost 1.92 tree and
reachable in every dependency mode:

| Mode | How `boost::span` arrives |
|---|---|
| internal | Boost.Core comes in transitively (algorithm → core). Add `core` to `BOOST_INCLUDE_LIBRARIES` (thirdparty/thirdparty-internal.cmake:6-17) so it does not depend on that. |
| Conan | boost/1.91.0 (conanfile.txt): `Boost::headers` (thirdparty-conan-build.cmake:28) carries every header. |
| external | headers come with any installed Boost, but `boost::span` needs **Boost ≥ 1.78**. `external_boost_deps.cmake` sets no minimum today, while Boost.JSON already needs 1.75. Add `find_package(Boost 1.78 …)` and the per-library `boost_core`. |

**Alternatives:**
- span-lite: a new pinned and hashed dependency. That means an entry in
  thirdparty/internal_deps.cmake with `URL_HASH`, a Conan requirement, an
  external-mode `find_package`, and updates to the install/export rules. It maps to
  `std::span` under C++20, which is nice but not needed.
- An internal span (about 60 lines): possible, but `boost::span` is already
  maintained, constexpr, and has C++20 `std::span` semantics.
- GSL: not needed. `NodeRef` replaces `not_null`; a five-line `narrow<T>` in
  src/helpers.h covers `gsl::narrow` (narrowing offsets and counts to 32 bits, with a
  check).

## 6. Tests that prove safety

**Checked refs:**
- Debug, the ASan+UBSan job (`-DJINJA2CPP_WITH_SANITIZERS=address+undefined -DCMAKE_BUILD_TYPE=RelWithDebInfo`,
  `ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1`) and the fuzz job all
  build with `JINJA2CPP_NODEREF_CHECKS=FULL`.
- Release CI keeps the default `ON`. One CI cell builds with `OFF`, so the
  configuration without checks still compiles and passes; it goes into the pairwise
  matrix as a new axis value.
- Unit tests that a corrupted ref is caught (a test-only hook that edits a sealed
  arena): the `Seal()` pass rejects it in `ON`, the per-access check in `FULL`; an
  expired escape handle gives `TemplateExpired` in all three modes.
- Locality gate for P3 and P4: cachegrind D1 and LL read misses (`valgrind
  --tool=cachegrind --cache-sim=yes`, an opt-in `count.py` column per the locality
  note) on many_tags, mitsuhiko_table and expressions. Read relative changes only.
- ASan cannot see per-node bounds inside one buffer. The checked headers do that job,
  and `ASAN_POISON_MEMORY_REGION` covers the slack and the headers in non-checked
  ASan runs.
- LSan catches leaks from the cleanup list or the side tables.

**TSan:** once locally for P4 (`-DJINJA2CPP_WITH_SANITIZERS=thread`). Renders must not
write to the arena or the side tables.

**Symbol table tests:** wide templates produce the same `SymbolId` for the same
name; synthesized names resolve; the attribute mirror matches the view;
`TemplateApiTest.ConcurrentRenderOfOneTemplate` under TSan shows no writes after
`Seal()`.

**Arena unit tests** (new test/node_arena_test.cpp):
- each check fails as expected (out of bounds, wrong type tag, wrong arena id, `Make`
  after `Seal`);
- the re-parse on overflow produces the same tree;
- the cleanup list runs in reverse order.

**Escape cases** (rows in existing tables; Python 3.1.6 checked):
- forloop_test.cpp: a `loop` kept in a namespace and called after the loop.
  `{% set ns = namespace(f=none) %}{% for x in [1] recursive %}{% set ns.f = loop %}<{{ x }}>{% endfor %}|{{ ns.f([7, 8]) }}`
  renders `<1>|<7><8>` (both engines).
- macro_test.cpp: a macro escaping the macro that defined it.
  `{% set ns = namespace(m=none) %}{% macro outer() %}{% macro inner(v) %}[{{ v }}]{% endmacro %}{% set ns.m = inner %}{% endmacro %}{{ outer() }}{{ ns.m(3) }}`
  renders `[3]`.
- Every arena-switch guard is exercised:
  - import_test.cpp:74 `MacrosOutliveUncachedModule`;
  - extends_test.cpp `UncachedParentsLiveThroughRender` (`super()` across three
    templates);
  - a new row: `self.block()` called from a child and from its parent;
  - a `caller` defined in one template and invoked from a macro imported from another;
  - include inside a macro from an imported template;
  - filesystem_handler_test.cpp:428 `EveryUseLooksUpOnEveryInclude`, plus a macro
    imported before the reload and called after it.
- template_api_test.cpp: a copy renders after the original is destroyed; extend
  `ConcurrentRenderOfOneTemplate` with `| map('upper')`, `| select('equalto', 't0')`
  and `| map(attribute='x')`.
- i18n_test.cpp: `_()` with arguments in a loop (the alias without nodes).
- The parity corpus is byte-identical in every phase.

**Ownership tests** (template_api_test.cpp and user_callable_test.cpp):
- **A stored macro called after its template is destroyed returns the error.**
  - Template A defines `m`. A user callable registered as an environment global
    receives `m` and stores the converted `Value`. Then A is destroyed.
  - Template B renders a call to the stored value and returns
    `ErrorCode::TemplateExpired`, with A's name and `m` in `ExtraParams`.
  - The same check for `caller` and for `loop` stored the same way.
- **A self-referencing template frees.**
  - A template that imports itself, and one that stores its own macro into an
    environment global through a user callable, both loaded through the environment
    cache.
  - After `~TemplateEnv` and after the user's `Template` goes away, LSan reports
    nothing and a counter (`weak_ptr::expired()` on a handle the test keeps) shows the
    impl is freed.
- **An `EveryUse` reload during a render:** the old include stays alive until the
  render ends (pin set), and is freed after it when no cache holds it (`expired()`).
- **Concurrent renders** (the existing test) under TSan: no refcount writes on the
  root's control block during renders.

**Fuzzing:**
- `fuzz_parse` covers partial trees, backtracking, the re-parse on overflow, and
  cleanup on error.
- The reload step from 0134 already runs on every input.
- Checked refs in the fuzz job turn a wrong guard or offset into an abort with a
  report.

## 7. What this makes hard next

- **Handing macros or modules to user code** (`Template.module`, a macro as a
  `UserCallable`): the escape path (`weak_ptr` + `NodeRef`, `TemplateExpired`) is
  the mechanism, so a module API becomes a conversion rather than an ownership change.
  What it cannot offer is a macro that keeps its template alive; users must keep the
  `Template`. That is deliberate.
- **Rewriting subtrees after Load:** replaced nodes stay in the arena.
- **Sharing subtrees across templates:** a `NodeRef` is meaningful only in its own
  arena. A shared subtree would need its own `TemplateSlot`.
- **Interning names across templates** (a process-wide symbol table, so thousands of
  cached per-tenant templates share name storage): a future measurement only, for
  servers with thousands of cached templates. With names as views into each
  template's source, the per-template symbol table is 16 B per distinct name. A global
  interner would bring back locking or a lock-free map on the `Load` path. Measure a
  template cache's total symbol bytes before considering it.
- **Tags or arenas over 4 GB:** a template over 4 GB of nodes fails `Load` with a clear
  error.
- **Easier:**
  - serialising or precompiling templates: offsets are already relocatable, so a
    sealed arena plus side tables is close to a file format;
  - footprint reporting: the arena size is the template's footprint.
- **Non-virtual dispatch** (a node-kind tag plus `switch`) would make arena objects
  trivially copyable. That enables relocation (no re-parse) and devirtualised
  rendering. It is a later, separate design.

## 8. Decisions for the owner

1. ~~What should a failed `Load` do?~~ Settled by 0134: it keeps the previous template.
2. **Values passed to user callables (lazy lists, `loop`, `self`, `context`): valid only
   for the duration of the call?** *Recommendation: yes; document it, add no
   keep-alive.*
3. **Is the -30% goal for 0118 alone, or for the series?** *Recommendation: the series,
   against 0109's 16.40M, with the gain recorded per PR.*
4. **Decided (owner, 07:51): `NodeRef` checks are on in every configuration**, with
   CMake option `JINJA2CPP_NODEREF_CHECKS`.
   - `ON` is the default in every configuration, Release included: refs verified at
     `Seal()`, plus range and generation checks on handles from outside the arena.
   - `FULL` is the default in Debug, ASan and fuzz builds: adds per-access range checks
     on every link, plus type and arena-id checks.
   - `OFF` is opt-in. It removes the offset-range checks (the `Seal()` pass and the
     per-access checks on handles) and the arena-generation compares.
   - The `weak_ptr` expiry check (`TemplateExpired`) and the slot bound stay
     regardless.
   - Budget for Release with `ON`: +0.5-1% on mitsuhiko_table and +1-2% on expressions,
     met by the indirection estimate. Per-access checks on every link measured too
     expensive for that budget (section 4A).
   - *Open sub-question: should `ON` be per access after all, accepting about +3% on
     mitsuhiko_table and expressions? Recommendation: no; keep that in `FULL`.*
5. **Drop structural equality of parse trees and the virtual `IComparable` base of
   renderers?** *Recommendation: yes, after a coverage run. It saves code and a vptr,
   and gives single inheritance for `NodeRef` casts.*
6. **One thread owns expression_evaluator.h, statements.h and template_parser.* for
   P3-P5?** *Recommendation: yes.*
7. **Remove node creation during renders entirely** (gettext alias, default arguments,
   the `Map` literal) instead of using local arenas? *Recommendation: yes, in P2a.*
8. **How should the arena grow?** *Recommendation: contiguous, sized from the rough
   parse, re-parse on overflow. Use the chunk table only if the measured re-parse rate
   exceeds 1%.*
9. **Where do refs resolve?** *Recommendation: the base in `RenderContext` (identity
   can be checked). Use self-relative refs if Render/expressions regresses by more than
   2% in P4.*
10. **Which span?** *Recommendation: `boost::span`, and raise external mode's minimum
    to Boost 1.78. No span-lite, no GSL.*
11. **What do escaping objects hold?** *Recommendation: inside a render,
    `{TemplateSlot, NodeRef}` handles checked through the context; on escape,
    `weak_ptr<const TemplateImpl>` + `NodeRef`, locked on use. Strong references only
    in `Template`, the cache and the render's pin set.*
12. **Render budget:** settled by the owner as +1-2% on expressions and +0.5-1% on
    mitsuhiko_table in Release with checks `ON` (decision 4). Revisit with section 7's
    dispatch work.
13. **Boost additions** (section 10): *Recommendation:*
    - *state `core` (span) and `range` explicitly;*
    - *Boost.Container's pmr monotonic resource as the allocation primitive, which
      needs `Boost::container` linked in the Conan and external modes;*
    - *no Flyweight (dropped per review), intrusive_ptr or PolyCollection; Boost.Intrusive
      is optional.*
14. **Symbol table scope.** Per-template and immutable after `Load` (the course
    correction), with a process-wide interner deferred to a measurement on large
    template caches? *Recommendation: yes.*
15. **Add `ErrorCode::TemplateExpired = 13`** to the public enum for calls through
    escaped callables whose template is gone? *Recommendation: yes. It is
    source-compatible; the alternative of reusing `TemplateNotFound` would misreport
    the cause.*
16. **How is the root pinned?** By the caller's `Template` (no refcount operation), or
    by a `shared_ptr` copy per render? *Recommendation: the caller's `Template`. A copy
    costs two atomics on one shared control block per render; measure with
    `--threads` MT/Render/plain_text only if the owner prefers the explicit copy.*
17. **Keep the environment handle in `TemplateImpl` strong** (the documented "template
    keeps its environment alive", with the cache loop broken by `~TemplateEnv` as
    today)? *Recommendation: yes. Making it weak would change public semantics and
    add a `lock()` per render.*

## 9. Ranges in the engine

**Child-list walks.**
- `for (auto& r : ctx.Nodes().Each(m_body))` over a `boost::span<const NodeRef<T>>`,
  with `boost::adaptors::transformed` to resolve each ref, reads like today's loop and
  checks every access.
- No type erasure, so it inlines; the cost is the measured +0.3%.
- Use it everywhere a body or argument list is walked (`ComposedRenderer`,
  `CallParamsInfo`, `TupleCreator`, `DictCreator`, `CompareExpression`).

**Parser token streams.**
- `LexScanner` is a cursor with saved positions for backtracking. `boost::sub_range`
  over the token vector would make lookahead slices explicit (balanced brackets in
  `ParseCallParams`, filter arguments) and remove index arithmetic.
- It is a readability gain with no performance effect, and does not change
  backtracking. Low priority; do it opportunistically in P3 where the code is touched
  anyway.

**List filters** (`select`/`reject`/`map`/`batch`/`slice`/`unique`).
- Today they are all eager: they enumerate through the `ListAdapter` enumerator into
  an `InternalValueList` (filters.cpp:723, 752, 902-911, 999, 1136, 1182, 1272). That
  is the reusable-lists divergence chosen in 0101; Python's versions are generators.
- Inside one filter, Boost.Range adaptors (`filtered`, `transformed`, `sliced`,
  `strided`) express the loop as a pipeline into the result list. That reads better
  and costs nothing, because it is statically typed and inlined.
- (`uniqued` removes only adjacent duplicates, so `unique` stays a hash-set pass.)

**Where type erasure costs more than today's enumerator.**
- Laziness across filters would mean storing a view in an `InternalValue`, which needs
  type erasure.
- `any_range<InternalValue, single_pass_traversal_tag, InternalValue>` costs three
  virtual calls per element (increment, equality with end, dereference), against the
  enumerator's two (`MoveNext`, `GetCurrent`).
- `any_range` copies iterators freely, and every copy clones the erased state. A
  pipeline capturing a predicate and the context overflows `any_iterator`'s buffer,
  which means a heap allocation per copy. The enumerator is moved, not copied.
- So: **no `any_range` at `InternalValue` boundaries.** If laziness is wanted (Python
  parity on `|select|first`, early stop), build it as a `ListAdapter` over the source
  enumerator (the existing `CreateAdapter(fn)` mechanism). That revisits 0101: the
  predicate would run again on each enumeration.
- Not measured. A scratch measurement of a lazy `select` was left out of this revision
  for time; the per-element call count above is the basis for the claim.

## 10. Boost libraries considered

| Library | What for | Verdict | Cost in BOOST_INCLUDE_LIBRARIES / Conan / external |
|---|---|---|---|
| Boost.Core `span` | child-list views | **use** | transitive already; add `core` explicitly / in `Boost::headers` / Boost ≥ 1.78 |
| Boost.Range | in-filter pipelines, list walks | **use, statically typed only** | transitive via algorithm; add `range` explicitly / headers / any version |
| Boost.Container `pmr::monotonic_buffer_resource` | bump allocation over the template's one buffer (`null_memory_resource` upstream) | **use, as the primitive only** | compiled library (libs/container/src/monotonic_buffer_resource.cpp): link `boost_container` (internal builds it already for json) / `container` in `find_package(Boost COMPONENTS …)` and `Boost::container` in `JINJA2_PRIVATE_LIBS_INT` / `find_package(boost_container)` + alias |
| Boost.Flyweight | interning names | **dropped** (owner's review): replaced by the per-template symbol table | header-only, but pulls MultiIndex, Interprocess and Parameter into the configure / headers / headers |
| Boost.SmartPtr `intrusive_ptr` | the `TemplateImpl` keep-alive | **no** | transitive / headers / headers |
| Boost.PolyCollection | storing nodes contiguously by type | **no** | header-only, add `poly_collection` / headers / headers |
| Boost.Intrusive | the arena's cleanup list | optional | transitive via container |

**pmr: how it is used, and how it is not.**
- Used: `monotonic_buffer_resource` over our one contiguous buffer, with
  `null_memory_resource` upstream. This gives a maintained bump allocator. On
  overflow it throws, and `Load` re-parses with a larger buffer.
- Not used, chunk growth: with a normal upstream the resource adds separate chunks
  and gives no offset mapping, so it could not back 32-bit offsets with one base.
- Not used, containers: `pmr::vector` and `pmr::string` store absolute pointers plus
  an 8-byte resource pointer each, 32 B or more against an 8-byte `ArenaSpan`. That is
  what the checked-handles principle replaces.
- Cost: every allocation is a virtual `do_allocate`, a few instructions, against
  today's malloc.
- Destructors: the resource frees memory without running destructors, so
  non-trivial objects still need the cleanup list. pmr does not remove it.
- `std::pmr` would need a macOS 14 deployment target with Apple libc++;
  `boost::container::pmr` avoids that.
- Possible later use: a thread-local monotonic resource for parse-time scratch
  (statement stack, token buffers, the interning map). Measure first.

**Why Flyweight was dropped.**
- Its factories are static per type, so interning is process-wide. The default
  `hashed_factory` with `simple_locking` takes a global mutex on every construction
  and every release, plus an atomic refcount: locking and refcounts on the `Load` and
  destruction paths.
- On `many_tags` (3,600 names, 307 distinct): an 8-byte handle would save about 72 KB,
  minus about 22 KB of global entries, for about +3-5% Load instructions (computed,
  not measured).
- The per-template symbol table saves about 85 KB, takes no lock, and carries the
  precomputed hash.

**Why each remaining "no":**
- **intrusive_ptr.**
  - Owning handles are needed only for objects that outlive a render. Inside a render,
    slot handles need no refcount at all.
  - `BasicTemplate`'s public `shared_ptr<ITemplateImpl>` (template.h:196) cannot
    change without breaking the ABI. Wrapping an intrusive count in a `shared_ptr`
    deleter would add a control-block allocation per `Template`.
  - Its one real advantage, an 8-byte handle that keeps `loop()` within libstdc++'s
    16-byte `std::function` buffer, is also met by `{slot, ref}`.
- **PolyCollection.**
  - Storage segmented by type breaks tree order, which is the render order. A body's
    children would be spread across segments, so locality gets worse, not better.
  - Its segments are growable vectors, so references would be {segment, index} and
    still need a resolver.
  - Tree walks dispatch per node anyway, so by-type batch iteration has nothing to
    batch.
- **Intrusive.** A `boost::intrusive::slist` of cleanup records living in the arena
  would avoid the cleanup vector's allocation (1 per template). A plain
  `std::vector<Cleanup>` is simpler. Pick either in P4.
