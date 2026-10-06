# Integrated design overview: 0117 + 0118 through a cache lens (2026-10-06)

Inputs:
- perf-cache-locality-notes.md (the brainstorm);
- 0118-parse-tree-arena-plan.md (Revisions 1-2);
- 0117-name-slots-plan.md (Revision 2026-10-06);
- repo master faea865;
- PR #414 (MiniJinja and Tera 2 against us; section 1a).

Rules that still hold:
- few raw pointers;
- `boost::span` and typed handles;
- one strong owner, weak escapes;
- the C++17 floor.

**Open with the owner, and not decided here:** the `NodeRef` check mode (validate at
`Seal()` versus every access). Section 3 gives the costs of both.

## 1. What the caches actually see today (measured)

Callgrind with `--cache-sim=yes` (it models the host's L1 32 KB and its last-level
cache, but not L2), over `CountedRegion` only. Release build of master faea865, built
in a scratch copy outside the repo. Figures are per operation, the counts divided by
5 iterations.

| Operation | Instructions | Data reads | D1 read misses | D1 write misses | I1 misses | Where the D1 read misses are |
|---|---:|---:|---:|---:|---:|---|
| Render/mitsuhiko_table | 7.26M | 2.59M | 8.1k | 5.2k | 0.7k | **82% user-list enumeration** (`ValuesListAdapter::GetCurrent`, reading 10k public `Value`s of 40 B), 12% variant visit. Write misses: 99% `memcpy` into the output string. The tree: about 0.3% |
| Render/expressions | 555k | 183k | 27 | 13 | 0.6k | none; bound by instructions (the operator tree walk) |
| Render/macros | 1.18M | 363k | 0.6k | 0.2k | **7.2k** | the I-cache: macro call path code (`InvokeMacroRenderer`, `BindSpecialMacroArgs`, `ScopeRef::operator[]`, hashtable, variant visits) |
| Render/many_tags | 1.29M | 404k | **13.6k** | 1.7k | 2.4k | **the tree**: `SubscriptExpression` 15%, `ExpressionRenderer` 13%, the root body walk 11%, `ComposedRenderer` 9%, `ValueRef` 6%, `RawText` 5%, `IfStatement`/`Set`/`Binary`/filters 4-5% each. 13.6k lines ≈ most of the 1.06 MB tree once per render |
| Load/many_tags | 16.46M | 4.75M | **58k** | 28k | **362k** | **42% inside malloc** (`malloc_consolidate` 29%, `unlink_chunk` 13%): walking the previous template's freed nodes. I-misses are spread over parser code (`FunctionBase::ParseParamsImpl` top, the lexer, `ParseSubscript`, `ParseFilterExpression`, ...) |
| Render/mitsuhiko_table_wide | 7.61M | 2.65M | 8.1k | **21.4k** | 5.6k | output writes (`wchar_t`, 4x the bytes) |

**Reading the table:**
- **Tree layout matters for single-pass, tag-heavy templates** (`many_tags`, chat
  templates, configs) and for Load. It does not matter for loop-heavy ones
  (`mitsuhiko_table`), whose loop bodies stay in L1.
- `mitsuhiko_table`'s misses are user data and output; no tree work touches them.
- `many_tags` runs at about 3.2 instructions per cycle on the wall clock (145 µs for
  1.29M instructions at 2.8 GHz). The prefetcher already hides most of the 13.6k
  misses, which is plausible because today's heap order is roughly parse order. So the
  render-time gain from a smaller tree is real but bounded.
- **The I-cache is the surprise.** Load takes 362k I1 misses per load, 2.2% of its
  instructions, and `macros` takes 7.2k per render. Code footprint per tag (parser +
  filter construction + malloc) and per macro call costs as much as data layout.

**Sizes measured on master** (GCC 13 x86-64): `InternalValue` 72, `InternalValueData` 48,
`TargetString` 40, `ListAdapter` 40, `Callable` 40, `CallParams` 104, `KeyValuePair` 104,
`RenderContext` 864, `OutStream` 2,088 (an inline buffer), `LookupCache` 3,080 (128 ×
24 B, thread_local), `LoopFrame` 576, `LoopState` 344, `TemplateFrame` 32,
`BlocksStack` 80. Node classes as in 0118 section 2 (`ComposedRenderer` 104,
`ExpressionRenderer` 104, `SubscriptExpression` 112, `BinaryExpression` 104,
`ForStatement` 184, `FunctionBase` 304, ...).

## 1a. Against the Rust engines (PR #414): fixed per-render cost and the Load gap

PR #414 ratios are engine time / ours. **MiniJinja:**
- Load: 0.41-0.68x everywhere except `large_static` (2.6x); `many_tags` 1.64 ms against
  our 3.37 ms.
- Render: wins `chat_qwen` 0.75x, `for_loop_vars` 0.75x, `config_file` 0.89x.

**Tera 2:**
- Render: wins `plain_text` 0.53x (136 ns against 258 ns), `substitute` 0.45x,
  `for_loop_vars` 0.42x, `inheritance` 0.62x.

**We lead:**
- `mitsuhiko_table` (MiniJinja 3.1x, Tera 1.5x);
- `expressions` (3.0x, 2.7x);
- `macros` (MiniJinja 3.2x).

So we lose where a render does little work per call, or where loop variables dominate.
We win where expression evaluation dominates. Measured on the same faea865 build (215 ns
for `plain_text` on this host, 2,130 instructions per render, `--count-iters=100`):

| Per-render step on `Render/plain_text` | Instructions | Share | Where |
|---|---:|---:|---|
| The template itself (one `RawTextRenderer`) | 33 | 1.5% | |
| **`self` map, built at every template entry**: `InternalValueMap self` + `CreateMapAdapter` (`make_shared`) + `try_emplace("self")` into the scope, whose robin_hood node pool allocates a block (`BulkPoolAllocator::performAllocation`) | 480 | 23% | `TemplateRenderer::RenderBody`, statements.cpp:1028-1042 |
| **…and its teardown** in `~RenderContext` (free the scope's node block, the adapter's control block, variant resets) | ~450 of 531 | ~21% | |
| **Env globals converted per render**: `ApplyGlobals` (a `shared_mutex` read lock even when there are no globals), `std::swap` of two maps, two map destructors, `GetBuiltinGlobals`, the `std::list<Value>` | ~315 | 15% | template_impl.h:232-262 |
| Output `reserve` (one malloc for the result string) + `Flush` | ~200 | 9% | template_impl.h:271-274; the result string is needed anyway |
| Block stack (`PushBlocks`, an `unordered_map` lookup), `RenderContext` constructor, callback, guards | ~150 | 7% | `RenderContext`'s 864 B construction itself is only 39 instructions; it is **not** a lever |
| `m_outputSizeHint` store | 1 instruction, but a shared write | | MT only (row 11) |

`Render/substitute` (`{{ message }} from Parser! - {{ number }}`, 408 ns, 4,025
instructions) adds a third fixed cost: **user parameters converted eagerly** into a
robin_hood map with `std::string` keys (`operator[]` 586 + `~Table` 432 instructions per
render for two parameters). This cost grows with the size of the context, used or not.

**What the fixed cost is, and what removes it** (new task N5, section 5):
- **N5a, lazy `self`:**
  - Resolve `self` from the current `TemplateFrame` only when a lookup asks for it,
    instead of materialising a map of block callables at every template entry. Each
    included, imported and parent template pays this today, so it also hits
    `inheritance` (`RenderBody` is 5.3% exclusive there, plus `try_emplace` 4.3%).
  - Python's precedence (checked with Jinja2 3.1.6):
    - `{% set self = 1 %}` wins over the template's own `self`;
    - the template's `self` wins over a global named `self`;
    - in an imported macro, `self` is the defining template (`self.b()` there raises
      UndefinedError when the block is in the importer).
  - The lookup must keep that order: scopes above the frame's base depth, then the
    frame's `self`, then globals.
- **N5b, env globals snapshot:**
  - Convert the env globals to an `InternalValueMap` once per change, not per render.
  - The snapshot is keyed by a process-wide generation number bumped by every globals
    mutation, so a destroyed env reused at the same address cannot match. The cache
    sits in the thread_local `RenderWorkspace` (row 10), so no refcount or other shared
    write is added.
  - The read lock is taken only to compare generations.
  - The gain grows with the number of globals, which real embedders have many of.
- **N5c, lazy parameter scope (measure first):**
  - The root scope looks names up in the caller's `ValuesMap` directly and converts on
    first use (by reference, as `InputValueConvertor(false, true)` already does).
    Lookups then hash once in a `std::unordered_map` instead of a robin_hood map; the
    symbol-keyed lookup cache (row 6) absorbs the repeats.
  - `GenericMap` keeps the eager path (it returns values by copy).
  - If 0117 P4 (root slots) lands first, N5c becomes "bind used parameters into root
    slots at render start", and this step is dropped.
- **N2** stops the shared write. The `RenderContext` itself stays: it is cheap to
  construct.

**Expected:** `plain_text` 2,130 → ~1,000-1,200 instructions, 215 ns → ~110-140 ns
(Tera 2: 136 ns). `substitute` → ~200-250 ns with N5c (Tera 2: about 184 ns). Medium
confidence for `plain_text`, medium-low for `substitute`. A residual 40-60 ns stays:
the C++ API returns a fresh `std::string`, so the `RenderAsString` malloc is
unavoidable.

**Where the Rust engines win larger renders:**
- **`for_loop_vars`** (Tera 0.42x, MiniJinja 0.75x):
  - `LoopAccessor::GetItem` (9.1%), `memcmp` (5.5%, mostly attribute-name compares),
    `LoopAccessor::Properties` (2.5%), `Subscript` (6%), and 100 mallocs per render.
  - **New in 0117 P1:** resolve `loop.<constant attribute>` to a `LoopAttr` enum at
    bind time, when `loop` binds to the loop slot. That removes the string dispatch.
    Recursive and escaped `loop` keep the dynamic path (0117 revision note).
  - Expected -20..-30% with the P1 slots, low-medium confidence. That passes MiniJinja
    but probably not Tera 2.
- **`inheritance`** (Tera 0.62x): 155 allocations per render, `self` per template level
  (N5a), and the block hashtable per level. Expected -10..-15% from N5a; the rest is
  0117 P2 and the lookup cache. A gap to Tera 2 likely remains (about 0.75x).
- **`chat_qwen`, `config_file`** (MiniJinja 0.75x, 0.89x): tag-dense single-pass
  templates like `many_tags`. They get the tree's cache gains (rows 3-4), the
  symbol-keyed cache (row 6) and N5. Expected -10..-20%; low confidence until N1 profiles
  them.

**The Load gap is not about bytecode.**
- MiniJinja's `Load` also builds a heap AST: every `Spanned<T>` boxes its node. Its
  codegen then emits a flat instruction vector and drops the AST.
- What it does not do is our per-tag work: 16.46M instructions for 2,400 tags is about
  6.9k per tag. Of that:
  - malloc, free and `operator new`: about 20%. 0118 P4/P5 removes it.
  - lexertk tokenisation into a `std::vector<Token>`, plus `Preprocess` and the vector's
    destruction: about 12%.
  - the separate rough parse (`FindBlockEnd`, `DoRoughParsing`, `ParseRoughMatch`,
    `StartControlBlock`), which re-scans what the lexer scans again: about 9%.
  - `expected<shared_ptr>` moves and destruction: about 2.5%. P3 makes this a 4-byte
    `NodeRef`.
  - `std::string` copies of names: about 2.2%. P5's symbol table removes them.
- Waves 1-2 take Load to about -30%, from 3.37 ms to about 2.3 ms: about 1.4x MiniJinja
  (medium-high confidence).
- **New task N6 (wave 3, plan first): a single-pass scanner** that finds tag boundaries
  and tokenises expressions in one forward pass, writing tokens into the arena. It
  attacks the remaining 21% and the parser's I-cache footprint. Expected a further
  -10..-15%, to about 1.15-1.25x MiniJinja (low confidence).
- Bytecode would add a codegen pass to Load. It is a render-time option (row 1), not a
  Load fix.
- `large_static` stays ours: raw text is a `SourceSpan` and is never copied.

## 2. Each earlier decision through the cache lens: keep or change

| # | Decision (source) | Verdict | Why |
|---|---|---|---|
| 1 | **Node representation: virtual classes in the arena** (0118) | **Keep for 0118. Add a 1-byte kind now. Tag+switch is a measured follow-up. Bytecode stays out.** | Render dispatch is not where the misses are: I1 misses at render are 0.7k (mitsuhiko) and 2.4k (many_tags) per render, and the loop dispatch is well predicted. minijinja and Tera 2 gain from bytecode with **small values** (minijinja's `Value` is 24 B) and enum dispatch. Our operand stack would move 72-byte `InternalValue`s, and the tree walk's by-reference `EvaluateRef` (0088) exists to avoid exactly those copies. **Once the arena exists, bytecode is cheaper to reach than we assumed, but not cheap.** The sealed tree (pre-order, offset links, kind byte, trivially destructible) is already tree-shaped bytecode, so switch dispatch can come per node family, and the kind byte is needed anyway for `Seal()` verification. A stack VM still needs a compiler pass, a VM and a value stack, and only pays after value shrink (row 9). Trigger for the follow-up: after row 9, if render I1 misses or indirect-branch cost exceed about 5% on `macros`/`expressions`, try switch dispatch on the 6 hottest kinds. |
| 2 | **32-bit `NodeRef` offsets** (0118 rev.) | **Keep, and stronger.** | The win now comes from node size: with hot nodes at 16-32 B, two to four nodes share a line. Measured indirection cost +0.1-0.4% on body lists. |
| 3 | **Child spans and pre-order placement** (0118 rev. 2) | **Keep, with one refinement.** | Fixed-arity children (binary, unary, filtered, subscript base) stay **inline refs**, not spans. A span costs an 8-byte header plus a separate line for 1-2 refs. Spans are for variable lists (bodies, call arguments, tuples, dict items, else branches). Pre-order statements with the body span right after the node make the root walk of `many_tags` (11% of its misses) sequential. |
| 4 | **Hot/cold split** (0118 rev. 2) | **Keep. The footprint target changes significantly.** | With the per-node budgets in section 3, `many_tags` comes to about **0.6 KB per line → 0.20-0.25 MB** (with constants and positions), against Revision 1's 0.35-0.45 MB. **That fits a 1-2 MB L2 with room for the source and the output.** Cold also covers the arguments-error text, display names, the data `IsEqual` would compare (if kept) and percent-format and `in`-literal caches (in the constants table). Source positions are stored only for nodes that can fail at render (calls, filters, subscripts, operators, tests): 8 B (`NodeRef`, source offset), about 12 per line. |
| 5 | **Symbol table layout** (0118 rev.) | **Keep.** | `Symbol{offset, length, hash}` = 16 B, four per line; 307 symbols on `many_tags` = 4.9 KB, entirely in L1 during the render. Narrow names are views into the source, so the strings sit in lines the template touches anyway. |
| 6 | **Lookup cache keyed by expression site** (0117 4.5: "the cache stays keyed by expression site") | **Change: key by `{TemplateSlot, SymbolId}` (or by the name's hash across templates), 32 entries. Gate on measurement.** | After 0117's slots, the cache serves only dynamic names (context and external variables, globals). Keyed by site, every one of `many_tags`' 2,400 sites gets its own entry and never hits within a render (each site runs once). Keyed by symbol, all reads of `obj` in an epoch share one entry: about 2 hits per line (≈ -3% of `Render/many_tags`, medium confidence; `set vN` starts a new epoch per line). The working set drops from 3,080 B to about 768 B (32 × 24 B), 2.3 KB of L1 back. The epoch rules are unchanged, since a lookup result depends only on (context, epoch, name). |
| 7 | **Slot frames** (0117) | **Keep. Change the `Slot` type.** | `std::optional<InternalValue>` is 80 B per slot. An "unbound" bit in `InternalValue`'s existing flag word (48 B data + 16 B parent + 8 B flags = 72 today) makes `Slot = InternalValue` at 72 B (-10%), and **makes value shrink (row 9) transparent to slot frames**: a slot becomes 32 B with no 0117 change. |
| 8 | **SoA scope map** (0117 P5) | **Keep, with byte budgets.** | A chunk of 16 keys (4 B `SymbolId`) = one 64 B line is searched; values are touched only on a hit. Values in a parallel array: 16 × 72 = 1,152 B today, 512 B after value shrink. Cross-template string names still take the hash path (0117 4.5). |
| 9 | **`InternalValue` 72 B** (not in either plan) | **New task, after 0117 P2 and 0118 P4; plan first.** | What 24-32 B would buy: slots and scope values 2.25x denser, lists 2.25x denser (10 values: 12 lines → 5), cheaper copies on every `Evaluate` by value, and it unlocks row 1's switch or bytecode option. What it would not buy: `mitsuhiko_table`'s misses, which read the user's public `Value`s (40 B, API, 0043). **Sequence after slots.** 0117 P1/P2 are independent and measured (-7..-10%, -20..-25%). With row 7's `Slot = InternalValue`, slots inherit the shrink for free. Shrinking first would delay the measured wins and conflict with the evaluator files 0117/0118 own. Expected -3..-8% on list/dict-heavy renders (`dict_ops`, `for_loop_vars`, chat templates), low confidence. The first step of its plan is to count copies and moves per render. |
| 10 | **Per-render state** (0117: per-render `SlotArena`; 0133: thread_local loop-frame pool; `OutStream` and `RenderContext` on the stack) | **Change: one thread_local `RenderWorkspace` with stack discipline, not a per-render arena.** | A per-render arena freed at render end loses 0133's reuse across renders and pays a first touch per render. The workspace holds the slot-frame buffer (grown to the largest seen, reused, mark and release per unit call), the loop-frame pool, and the lookup cache (row 6), all adjacent and warm across renders on the thread. Stack discipline (mark/release) keeps a render re-entrant when a user callable renders another template on the same thread. `RenderContext` (864 B) and `OutStream` (2 KB buffer) stay on the stack, which is already the hottest memory. Handles into it are `FrameHandle`s (offset + generation, 0117 4.2), never pointers. |
| 11 | **MT: `m_outputSizeHint`** (template_impl.h:617, stored at :277 on every render) | **New small task, first wave.** | Every render, on every thread, writes the line that also holds `m_renderer` and `m_templateName`, which every render reads. That is false sharing, about one coherence miss per render per core. It is small on `mitsuhiko_table` and up to about 10-15% of a 224 ns `plain_text` render at 4 threads (estimate). Fix: store only when the hint changes by more than 25%, and put the atomic on its own line (`alignas(64)`). Measure with `--threads` MT/Render/plain_text, substitute and inheritance. |
| 12 | **MT: `shared_ptr` refcounts per render** (0118 rev.: the root is pinned by the caller's `Template`; includes are pinned in `Loaded()`) | **Keep.** | The root takes no refcount operation. Each included, imported or parent template costs one inc/dec pair per render on its shared control block. Acceptable, and measured by the MT inheritance bench (0105: 70-81k renders/s on 4 threads). Revisit only if that scaling drops below linear-minus-10%. |
| 13 | **Arena allocation and Load** (0118) | **Keep. The case for it is stronger.** | 42% of Load's D1 read misses are inside malloc's free-list walks, which the arena removes, along with malloc's code from the per-tag I-cache footprint. |
| 14 | **I-cache footprint of Load and macro calls** (not covered) | **New measured goal inside existing phases.** | Load: 362k I1 misses per load. 0130 P2a (compact filter construction without `std::vector<ArgumentInfo>` copies) and 0118 P4 (no malloc and `shared_ptr` paths) shrink per-tag code. Macros: 0117 P2 replaces `BindSpecialMacroArgs`/`ScopeRef`/hashtable paths with slot writes. Gate: report I1 misses in the cachegrind columns, and do not accept a PR that raises them more than 5% on Load/many_tags or Render/macros. PGO remains the embedder's lever for layout (bench README: -12-15% Load). |
| 15 | **Fixed per-render cost** (not in either plan; PR #414: Tera 2 renders `plain_text` in 0.53x our time) | **New task N5 in wave 1** (section 1a). | 98% of a `plain_text` render is setup and teardown. The levers, in order of size: the `self` map built at every template entry (~44% with its teardown; N5a lazy), env globals converted per render (15%; N5b snapshot in the workspace), eager parameter conversion (substitute; N5c, or 0117 P4). `RenderContext` construction is not a lever (39 instructions). |
| 16 | **Load gap against bytecode compilers** (PR #414: MiniJinja 0.41-0.68x) | **Explained; not a reason for bytecode.** New task N6 (single-pass scanner) in wave 3. | MiniJinja also allocates an AST. Our excess is per-tag work: the allocator (20%, 0118), two scans (lexertk + rough parse, about 21%, N6), and `shared_ptr`/`expected`/`std::string` traffic (about 5%, P3/P5). |
| 17 | **`loop.<attr>` by string** (0117 P1 binds `loop` but keeps string attribute dispatch) | **Change: resolve to a `LoopAttr` enum at bind time.** | `for_loop_vars`: `LoopAccessor::GetItem` + attribute `memcmp` + `Properties` ≈ 17% of the render. Recursive and escaped `loop` stay dynamic. |

**Significant changes:**
- row 4 (footprint target 0.20-0.25 MB, fits L2);
- row 6 (lookup cache rekeyed by symbol, shrunk to 32 entries; 0117 said by site);
- row 7 (`Slot` = `InternalValue` with an unbound bit, not `std::optional`);
- row 10 (thread_local `RenderWorkspace` instead of a per-render arena);
- row 9 (new value-size task sequenced after slots);
- row 11 (a new false-sharing task in the first wave);
- row 15 (new task N5: the fixed per-render cost, mostly the `self` map and the globals
  conversion; this is where Tera 2 beats us on small renders);
- row 16 (the Load gap is per-tag work, not bytecode; new task N6, a single-pass
  scanner);
- row 17 (`loop.<attr>` resolved to an enum in 0117 P1).

Row 1 keeps the virtual nodes but adds the kind byte and states when switch dispatch
is worth trying.

## 3. Data layouts and byte budgets

**Parse tree** (one contiguous arena per template; offsets `uint32`; header = vptr 8 +
kind 1 + flags 1 + pad 2):

| Node | Today (object + 16 B control block + malloc ≈ 8-16) | Budget | Hot fields |
|---|---:|---:|---|
| `ValueRefExpression` | 48 → ~72 | **16** | header, `SymbolId` |
| `ConstantExpression` | 80 → ~104 | **16** | header, `ConstRef` |
| `RawTextRenderer` | 56 → ~80 | **16** | header, `SourceSpan` (long flag in flags) |
| `ExpressionRenderer` | 104 → ~128 | **16** | header, expression ref (`finalize` gone, 0130) |
| `ComposedRenderer` | 104 + heap growth | **16 + 4/child** | header, `ArenaSpan` right after it |
| `FilteredExpression` + `ExpressionFilter` | 40 + 72 → ~150 | **16** | header, operand ref, filter ref |
| filter object (`FunctionBase`) | 304-424 → ~330-450 | **24-32** | vptr, `ArgSource` span, mode; error as a code |
| `SubscriptExpression` (one index) | 112 → ~136 | **24** | header, base ref, index (`SymbolId` or ref, isAttr, maybeMethod in flags) |
| `BinaryExpression` | 104 → ~128 | **24** | header, op, by-ref bits, 2 inline refs; `in`-literal and format → `ConstRef` |
| `IfStatement` | 88 → ~112 | **24** | header, condition, body, else span |
| `SetLineStatement` | 128 → ~152 | **16-24** | header, target (`SymbolId` or tuple span), expression |
| `ForStatement` | 184 → ~208 | **40** | header, target, value, filter, body, else, recursive flag, loop id (8 B: 0133 needs it unique per process) |
| `MacroStatement` | 112 + params | **32 + 12/param** | header, `SymbolId`, body, param span (`SymbolId`, default ref, flag) |
| symbol | 32 B `std::string` per use | **16 per distinct name** | offset, length, hash |
| constant | 72 + node | **72 per distinct constant**, interned (shrinks with row 9) | `InternalValue` in the side table |
| source position (cold) | none today (render errors report 1:1) | **8 per fallible node** | `NodeRef`, source offset, sorted by allocation |

**`many_tags` per line:** ~550 B of nodes + ~70 B of constants + ~100 B of positions ≈
0.6-0.7 KB, against 3.5 KB today. **Retained: 0.20-0.25 MB including the 39 KB source.**
The arena slack after the rough-parse estimate (≤ 25%) is already in the high end.

**Render time:**

| Structure | Today | Budget | Where it lives |
|---|---:|---:|---|
| `RenderContext` | 864 | 864 → about 400 after 0117 P2 (fewer scope chunks inline) | stack |
| `OutStream` buffer | 2,088 | unchanged | stack |
| lookup cache | 3,080 (128 × 24) | **768 (32 × 24)**, keyed by symbol | thread_local `RenderWorkspace` |
| slot | 80 (`optional<InternalValue>`) | **72**, then **32** after value shrink | workspace buffer, mark/release |
| loop frame | 576 (pooled, 0133) | unchanged in size, pooled in the workspace | workspace |
| scope chunk (SoA) | robin_hood node map | 64 B keys + 16 × value | workspace or stack |
| `TemplateImpl` hot line | `m_renderer` next to the written `m_outputSizeHint` | hint on its own 64 B line | heap (owner) |

**NodeRef check modes (open with the owner):**
- **Validate at `Seal()`:** about 3 instructions per stored ref at Load (about +0.2-0.5%
  Load), zero at render.
- **Every access:** measured +1.36% on mitsuhiko_table for body lists alone, about +2.5-3%
  for every link (+2-3% expressions).
- Both keep the `weak_ptr` expiry check (`TemplateExpired`) and the slot bounds.
- Neither changes the layouts above, except the checked 8-byte layout for `FULL`
  builds.

## 4. What changes in the two plans

**0118:**
- P3 must add the kind byte (row 1). Fixed-arity children stay inline (row 3).
- The retained target tightens to 0.20-0.25 MB (row 4).
- The lookup-cache key moves to `{TemplateSlot, SymbolId}` (row 6, shared with 0117).
- Cachegrind and I1 gates apply to P3-P5 (row 14).

**0117:**
- `Slot` = `InternalValue` with an unbound bit (row 7).
- `SlotArena` becomes part of a thread_local `RenderWorkspace` with mark and release
  (row 10). `FrameHandle` stays.
- The cache is rekeyed by symbol after P1/P2, measured (row 6).
- The P5 SoA budgets (row 8).
- P1 also resolves `loop.<constant attribute>` to a `LoopAttr` enum (row 17).
- P4 (root slots) absorbs N5c if it lands first (section 1a).

## 5. Revised phase order (0117 + 0118 + new tasks)

| Wave | PR | Notes |
|---|---|---|
| **1** (independent; parallel threads) | **N1 cachegrind gate**: `count.py --cache-sim` adds D1mr, D1mw, I1mr columns (callgrind `--cache-sim=yes`, as measured here) to output and the trend; opt-in in the PR gate | first, so everything after is measured |
| | **N2 output-hint false sharing** (template_impl.h) | tiny; same thread as N5 (same file) |
| | **N5a lazy `self`** (statements.cpp `RenderBody`, render_context.h lookup), **N5b env-globals snapshot** (template_env, template_impl.h; workspace slot or a thread_local until the workspace exists) | `plain_text` ~-45%, `substitute` ~-30%, `inheritance` -10..-15% |
| | 0131 P1 (Load fixed cost) | |
| | 0130 P2a (compact `FunctionBase` + `ArgSource`; no render-time default nodes; gettext without nodes), P2b (`finalize`, visitor vptr, root shrink) | also the first hot/cold steps; I-cache |
| | 0117 P0 (parity: bound-scope order), P1a (`LookupResult`) | P1a before 0118 P3 (expression_evaluator.h) |
| **2** (one thread owns expression_evaluator.h, statements.h, template_parser.*, render_context.h) | 0118 P3: `NodeRef`/`ArenaSpan`/`SymbolId`/`ConstRef` API, kind byte, hot/cold, pre-order, inline fixed-arity children | neutral ±0.5% |
| | 0117 P1: for/loop slots, `Slot = InternalValue`+unbound bit, `LoopAttr` enum, `RenderWorkspace` (slot buffer + 0133 pool; takes over N5b's cache) | mitsuhiko -7..-10%; for_loop_vars -20..-30% |
| | 0118 P4: arena, slots of the template pin set, escapes, checks (mode per the owner) | Load -18..-22%; D1 misses on Load -40% |
| | 0117 P2: macro, call and with params, `super` | macros -20..-25%; I1 misses on macros down |
| | 0118 P5: trivially destructible nodes, symbol table, constants, positions | Load total -30%; retained 0.20-0.25 MB |
| | lookup cache rekey + shrink (row 6) and 0117 P5 SoA `ScopeMap` | measure each; many_tags, macros, chat_* |
| | 0117 P3 (set targets; measure first) | may be rejected |
| **3** (plan first) | **N3 `InternalValue` to 24-32 B** | after wave 2 |
| | **N4 switch dispatch on hot kinds** (experiment) | only if row 1's trigger fires after N3 |
| | 0117 P4 (after 0038) | root slots; many_tags -15..-25%; absorbs N5c |
| | **N5c lazy parameter scope** (only if 0117 P4 is not scheduled) | substitute, chat_*: proportional to the context size |
| | **N6 single-pass scanner** (template_parser.*, lexer.*; after 0118 P5) | Load a further -10..-15% |

## 6. Expected gains and confidence

| Metric | Expected after wave 2 | Confidence | Basis |
|---|---|---|---|
| Load/many_tags instructions | -30% (16.46M → ≤11.5M) | medium-high | 0118 profile: allocator 21%, destruction 9.6%, filter construction 5.7% |
| Load/many_tags D1 read misses | -40..-50% | high | 42% of them are in malloc's free-list walks |
| Load/many_tags I1 misses | -20..-30% | medium | malloc and `shared_ptr` code gone, compact filter construction |
| many_tags retained | 1.06 MB → 0.20-0.25 MB | medium | section 3 budgets |
| Render/many_tags D1 read misses | 13.6k → 3-4k per render | medium | the tree is 4-5x smaller and walked forward |
| Render/many_tags time | -5..-15% | low | the prefetcher already hides much (IPC about 3.2) |
| Render/mitsuhiko_table | -6..-9% net (0117 P1 -7..-10%, `NodeRef` +0.5-1%) | medium-high | misses are user data and output; the tree is irrelevant |
| Render/macros | -20..-25% | medium | 0117 P2, plus fewer I1 misses |
| Render/expressions | -2..-3% net (0117 -4%, `NodeRef` +1-2%) | medium | instruction-bound; the lever is N3/N4 |
| MT plain_text/substitute at 4 threads | +5..+15% throughput | low-medium | N2; measure with `--threads` |
| N3 value shrink | -3..-8% on list/dict renders | low | plan first, counting copies |
| **Render/plain_text** (N5a, N5b, N2) | 2,130 → ~1,000-1,200 instructions; 215 → ~110-140 ns (Tera 2: 136 ns) | medium | section 1a breakdown |
| **Render/substitute** (+ N5c or 0117 P4) | 408 → ~200-250 ns (Tera 2: ~184 ns) | medium-low | parameter conversion is 1,000 of its 4,025 instructions |
| **Render/inheritance** (N5a) | -10..-15%; the gap to Tera 2 narrows from 0.62x to about 0.75x | low-medium | `self` per template level |
| **Render/for_loop_vars** (0117 P1 + `LoopAttr`) | -20..-30%: ahead of MiniJinja, still behind Tera 2 | low-medium | string dispatch ≈ 17% of the render |
| **Render/chat_qwen, config_file** | -10..-20% | low | tree layout, cache rekey, N5; profile under N1 first |
| **Load/many_tags against MiniJinja** | after wave 2: 3.37 → ~2.3 ms (1.4x MiniJinja); after N6: ~1.9-2.0 ms (1.15-1.25x) | medium-high / low | section 1a |

## 7. Open questions for the owner (new or changed)

1. **NodeRef check mode:** validate at `Seal()` versus every access (still open; costs
   in section 3).
2. **Rekey the lookup cache by symbol** (0117 had it by site)? *Recommendation: yes,
   measured on many_tags, macros and chat_*.*
3. **`Slot` = `InternalValue` with an unbound bit** instead of `std::optional`?
   *Recommendation: yes.*
4. **A thread_local `RenderWorkspace` instead of a per-render arena?**
   *Recommendation: yes.*
5. **N3 (value size) after wave 2, with its own plan?** *Recommendation: yes. Bytecode
   is not considered before N3.*
6. **N1 and N2 in wave 1?** *Recommendation: yes.*
7. **N5 (per-render fixed cost) in wave 1, with N5c deferred to 0117 P4?**
   *Recommendation: yes. N5a needs parity rows for `self`'s precedence (section 1a).*
8. **N6 (single-pass scanner) as the Load follow-up instead of bytecode?**
   *Recommendation: yes, after 0118 P5.*
