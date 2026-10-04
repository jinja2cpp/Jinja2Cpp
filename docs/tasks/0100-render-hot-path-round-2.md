---
status: in-progress
priority: medium
area: perf
depends: [0088]
touches: [src/expression_evaluator.cpp, src/expression_evaluator.h, src/renderer.h, src/template_impl.h, src/internal_value.cpp, src/filters.cpp, src/render_context.h]
---
# Render hot path, round 2

**Problem.** After 0088 (#362) Jinja2C++ beats Python Jinja2 on every Render benchmark
(expressions 1.17x, dict_ops 1.33x, mitsuhiko_table 1.40x, the rest 1.7-10x), but the
profile is flat again and these ideas from it were left for later. Instruction shares
are from callgrind on the Release benchmark after #362.

**Ideas, ordered by payoff per risk.** Measure each with `bench/count.py --baseline`.

1. *User data conversion.* `ValuesListAdapter::GetItem` converts each `Value` to an
   `InternalValue` through `InputValueConvertor` (mitsuhiko_table: `GetCurrent` 12%
   inclusive). A fast path for scalar alternatives (int, double, bool, string by
   reference) skips the visitor. Low risk; about -5% on mitsuhiko_table.
2. *Output reserve.* The result string regrows while rendering (`_M_mutate` plus
   copies, about 3-5% of mitsuhiko_table). Reserve from the previous render's size,
   kept as a relaxed atomic hint in `TemplateImpl` (templates render concurrently).
3. *dictsort and map iteration.* `DictSort::Filter` takes `GetKeys()` and then looks
   each key up again with `GetValueByName` (two hash lookups and a key vector per
   item); an entry enumerator on `IMapAccessor` would read pairs directly. About -10%
   on dict_ops; touches every map accessor.
4. *Attribute of a map.* `config.key` goes `LookupIndex` → `Subscript` → visitor →
   `MapAdapter::GetValueByName` (dict_ops: `SubscriptExpression::Evaluate` 19%
   inclusive). A direct path for a `MapAdapter` with `KeysOnly`/plain policy and no
   method of that name. About -8% on dict_ops; must keep 0020's method/key order.
5. *Flatter `{{ x }}`.* `ExpressionRenderer` → `FullExpressionEvaluator::Render` →
   `ExpressionEvaluatorBase::Render` → `EvaluateRef` is three virtual calls before the
   lookup. The parser can store the inner expression when there is no inline `if` and
   no finalize. About -2%.
6. *`is` tests and the inline `if` by reference.* `i is even` and `'y' if c else 'n'`
   still copy the operand (`ValueRefExpression::Evaluate` 8% of expressions). Tests
   without arguments can take `EvaluateRef`. About -3% on expressions.
7. *Variable lookup cache.* `FindValue` is 12-13% of mitsuhiko_table and expressions.
   A per-render inline cache (slot pointer plus a scope generation counter bumped on
   enter/exit/insert) would make a hit about 10 instructions. The AST is shared across
   threads, so the cache must live in `RenderContext`, keyed by a per-template node
   number: this is most of the way to slot resolution, which 0088 rejected until 0038
   settles scoping. Medium-high risk; about -10%.
8. *Typed lowering of arithmetic subtrees.* Compile `(i * 3 + 7) // 2 % 11` into a
   closure over int64/double without `InternalValue` temporaries, falling back to the
   generic path on any non-number. Only expressions-style templates gain (maybe -20%);
   high effort.
9. *Allocation per loop and per row.* malloc/free are still 7-10% of dict_ops and
   mitsuhiko_table: an enumerator (`ValuePtr`) and an adapter (`make_shared`) per
   inner loop, the dictsort result list. Pool or embed them once the above settle.

**Done when.** The ideas above are each landed or rejected with a measurement, and the
results are recorded here.

## Results

Instructions per render from `bench/count.py`, Release build, change against master
9548fa2.

| Idea | Landed in | Measured |
|---|---|---|
| 1. User data conversion | PR (part A) | Landed as `GetCurrentItem`: the enumerator no longer wraps each item in `std::optional`. mitsuhiko_table -8.1%, for_range -9.1%, dict_ops -5.0%. The scalar fast path itself was worth only -0.6%: the optional was the cost. |
| 2. Output reserve | | Waits for 0104 (same function). |
| 3. dictsort entries | PR (part A) | `IMapAccessor::GetEntries`, dictsort sorts pointers. dict_ops -15.4%. |
| 4. Attribute of a map | PR (part A) | `Subscript(value, name)` reads a mapping directly, no `HasValue` before `GetValueByName` for the engine's own maps (user `IMapItemAccessor`s are still asked), attribute key built only on a miss. for_filter_if -16%, for_loop_vars -16%, many_tags -9%, inheritance -6%, dict_ops -5%. |
| 5. Flatter `{{ x }}` | PR (part A) | -0.1 to -0.5% Render; Load within ±0.7% (one `dynamic_cast` per output tag). |
| 6. `is` and inline `if` by reference | PR (part A) | expressions -6.1%. |
| 7. Variable lookup cache | Step 1 in PR (part A) | Architect plan: (1) cheaper walk: word-wise `NameEqual` for names up to 16 bytes and a plain backward scope loop, mitsuhiko_table -2.1%; (2) route every scope write through a `ScopeRef` type so the compiler finds them; (3) a per-render inline cache (per-template node ids, per-context epoch bumped on insert/clear/exit/bind, Debug cross-check against `FindValue`), estimated mitsuhiko -6%, expressions -8% more. Steps 2-3 open (part B). |
| 8. Typed arithmetic | | Open. |
| 9. Allocations per loop | | Open. Per row: `LoopState`, `LoopAccessor`, the enumerator, the row's adapter, plus robin_hood's table and node chunk for the loop scope. Plan: `LoopAccessor` inside `LoopState`, pool popped scope maps (after step 3 of idea 7), index-based iteration of indexed lists without a loop filter. |

Part A together: dict_ops -24%, for_filter_if -18%, for_loop_vars -17%, mitsuhiko_table
-11%, for_range -10%, many_tags -10%, expressions -9%, inheritance -7%, strings -6%,
macros -5%; nothing slower. Load within ±0.7%.
