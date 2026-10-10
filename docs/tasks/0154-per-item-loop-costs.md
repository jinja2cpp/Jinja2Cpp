---
status: open
priority: medium
area: perf
depends: [0117, 0118]
touches: [bench/jinja2cpp_bench.cpp, src/expression_evaluator.cpp#SubscriptExpression, src/internal_value.cpp#Subscript, src/internal_value.cpp#ValuesMapAdapter, src/value_visitors.h#InputValueConvertor, src/statements.cpp#ForStatement, src/statements.cpp#IncludeStatement, src/template_impl.h#RendererCallback]
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
