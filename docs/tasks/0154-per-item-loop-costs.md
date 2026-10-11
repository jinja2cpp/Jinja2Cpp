---
status: done
priority: medium
area: perf
depends: [0117, 0118]
touches: [src/robin_hood.h, src/expression_evaluator.cpp#SubscriptExpression, src/internal_value.cpp#ValuesListAdapter, src/statements.cpp#ForStatement, src/statements.cpp#IncludeStatement, src/template_impl.h#RendererCallback, src/render_context.h#IRendererCallback, src/value_methods.cpp]
---
# What each item of a loop over user data costs that Tera does not pay

**Problem.** Found re-profiling the Tera 2 gaps (0136) on master 50dec81: Tera renders
`for_filter_if` in 0.73x of our time and `inheritance` in 0.74x. Both loop over a list of
user maps (`users`, 200 items; `items`, 50) and read a few attributes per item. Callgrind
(`jinja2cpp_bench --count`, Release), self cost of each part:

| Part | `for_filter_if` | `inheritance` |
|---|---:|---:|
| Attribute read `item.x` on a user map (`SubscriptExpression` → `Subscript` → `ValuesMapAdapter::GetItem`, hashing, `memcmp`) | 31.3% | 23.1% |
| The filtered-loop adapter (`CreateSlottedFilteredAdapter` lambda, a `std::function` per item) | 12.9% | |
| malloc, free, `operator new`/`delete` | 7.1% | 6.0% |
| Per-`include` setup (`LoadTemplate` by name, `RenderContext::Clone` and its destructor, the constant name, `ConvertToList`) | | 10.0% |
| `FindValueWithViews` (an include in a loop walks the loop's view: phase 6 of 0118/0117, not this task) | | 12.6% |

- **Attribute reads** cost about 320 instructions each (`SubscriptExpression::Evaluate`
  inclusive, 24,856 calls in `for_filter_if`): about 130 in the `Evaluate` →
  `ApplyFirstIndex` → `LookupDefinedIndex` layers, 160 in `Subscript(value, string)` and its
  visitor, 96 in `ValuesMapAdapter::GetItem` (hash, find, convert the `Value`). 0115
  added the one-index fast path; the layers and the conversion remain.
- **Map items allocate.** Enumerating a user list converts each item; a `ValuesMap` item
  becomes a heap-allocated map adapter (`InputValueConvertor` on the
  `polymorphic<ValuesMap>` alternative): 10,400 allocations over 50 renders, one per
  `users` item.
- **The filtered loop** (`for u in users if u.active`) wraps the list in a lazy adapter
  so that `loop.length`, `loop.last` and `loop.revindex` count only the kept items, as
  in Jinja2. Tera's translation filters with an `if` in the body. A body that reads none
  of those attributes could filter in place.
- **Each `include`** in the loop looks its template up by name in the render's cache
  (about 350 instructions: a `std::string` of the name and an `unordered_map` find,
  though the name is a constant) and clones the context.
- **The allocation counts miss part of the heap.** robin_hood (`src/robin_hood.h`)
  allocates its tables and node pools with `std::malloc`, which the bench driver's
  `operator new` counter does not see: `Render/substitute` reports 1 allocation per
  render and makes 3 (the parameter map's table and node pool; callgrind's self cost of
  `malloc` agrees). `Retained` and `Peak` (0121) miss the same blocks.

**Proposal.** Measure each with `bench/count.py --baseline`, after wave 2 (0117/0118
own expression_evaluator.* and statements.*):
1. Count robin_hood's blocks first: route its allocations through `operator new` (a
   configurable allocator in the vendored header, or a wrapper), or hook `malloc` in the
   driver, so the allocation and memory columns cover them. Expect a one-time step in the
   trend (note it as 0121 did).
2. Lend a user map item to the enumerator by reference (no adapter allocation) while
   the list is alive, as 0115 did for strings; check the lifetime rules of 0112/0125.
3. Flatten the attribute path for the common case: a name subscript on a map adapter
   goes straight to the accessor's `Find(string_view)` with the precomputed hash, without
   the generic `Subscript` visitor.
4. Filter in place when the loop body (and its `else`) reads no attribute that needs the
   filtered length (`length`, `last`, `revindex`, `revindex0`, `cycle` is fine), decided
   at Load like 0117's `LoopAttr` resolution.
5. Resolve a constant `include` name once per render per node (a `TemplateHandle` in the
   render's slots, 0118 P4b) instead of by name per use, keeping
   `TemplateLookup::EveryUse` behaviour (0105).

**Done when.** `Render/for_filter_if` and `Render/inheritance` at or below Tera's time in
the bench/README.md table (with phase 6 merged), or each part above measured and its
remaining cost explained here.

**Done** (split per the Performance track into #454 (robin_hood counting), #456 (lazy filtered loops) and part C (steps 3-5, the next PR on the same branch); measured as a whole on master 2a844f9; `count.py`, Release, instructions per render):

| Case | Before | After | Allocations |
|---|---:|---:|---:|
| `Render/for_filter_if` | 447,213 | 340,292 (-23.9%) | 206 -> 6 |
| `Render/inheritance` | 194,106 | 149,755 (-22.9%) | 73 -> 34 |
| `Render/mitsuhiko_table` | 6,623,767 | 6,555,230 (-1.0%) | 1,025 -> 35 |
| `Render/config_file`, `html_autoescape`, `macros`, `many_tags` | | -2.2..-2.7% | |
| `Render/chat_mistral`, `chat_qwen` | | -1.6% | |

Allocations count robin_hood's blocks from step 1 on (one-time step: +2 per render on most
cases, `many_tags` +13 and 76 KB of peak that were invisible before).

1. robin_hood allocates through `::operator new`/`delete` (a second local change, noted at
   the top of `src/robin_hood.h`).
2. A filtered loop over slots fetches lazily, as Jinja2's `LoopContext`: the filter runs in
   place (`ForStatement::FilterInFrame`) when the body is done with an item, and
   `last`/`nextitem`/`length`/`revindex` peek through the frame-resolving path
   (`FetchFiltered`) only when read. This fixes `statements.for_filter_sees_body_namespace`
   (0091). Loops in a recursive loop (`RenderLoopInScopes`) keep the eager filter adapter.
   for_filter_if -15%.
3. A user list (`ValuesListAdapter<ByRef>`) lends its list and map items from blocks of 256
   adapters (aliasing `shared_ptr`), instead of `make_shared` per item read. The list keeps
   only the current block, so peak memory stays bounded. for_filter_if -7%, mitsuhiko -1.5%.
4. `x.name` on a map calls `Subscript` directly from `SubscriptExpression::Evaluate`, also
   for names only other kinds have methods of (`item.title`; the parser now records
   `maybeDictMethod`). for_filter_if -4%, inheritance -8%.
5. An `include` of a constant name keeps what it loaded per render per statement
   (`IRendererCallback::FindLoadedBy`, a short list in the render's `LoadedTemplates`;
   EveryUse unchanged). inheritance -13.6%.

**What is left** (callgrind on the result):
- `for_filter_if`: an attribute read costs about 200 instructions, 100 of them in
  `std::unordered_map::find` on the user's `ValuesMap` (hash and compare of the name),
  which only a different `ValuesMap` (0043) removes; the rest is moving the `InternalValue`
  out through `Subscript`. The in-place filter costs about 190 per item outside the
  filter expression: binding the item into the filter slot and moving it back out (two
  variant move-assigns that release the previous items' adapters), plus a view push and
  two epochs per kept item. Binding the slot by reference would save the moves; not done.
- `inheritance`: `FindValueWithViews` (12%, an include in a loop walks the loop's view:
  0117/0118 phase 6) and `RenderContext::Clone` with its destructor (about 170 per include,
  render_context.h).
- Tera's time was not measured here (no Rust toolchain in the cloud): by instruction ratio
  the two cases are at 0.76x and 0.77x of the old time, against Tera's 0.71x and 0.74x.

**Trade-offs and obligations**:
- Retained memory: an item of a user list that escapes its loop (stored through `set` or a
  namespace, returned by a filter, captured by a callable) keeps its whole block of 256 lent
  adapters alive, and with it the user list.
- Peak bytes: fewer allocations, but bigger ones. A block allocates up to 256 adapter slots at
  once (one variant each, list or map), so peak memory per render rises: for_filter_if
  goes from 2,840 B to 13,904 B, and mitsuhiko_table rises about 27 KB (CI job 114358955252).
  Nothing is retained after the render. This is accepted because the block replaces one
  allocation per item, and its size is bounded by 256 items whatever the list's length (one
  block per list, tried first, cost mitsuhiko_table +55 KB).
- 0140 P2 obligation: the block (`ItemAdapters` and `ValuesListAdapter::LendNested` in
  internal_value.cpp, the only place that makes these aliasing pointers) becomes one
  refcounted ValueObject block with each lent item an intrusive pointer into it. The 0140
  thread takes this over in P2.
