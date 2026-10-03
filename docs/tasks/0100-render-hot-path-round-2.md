---
status: open
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
