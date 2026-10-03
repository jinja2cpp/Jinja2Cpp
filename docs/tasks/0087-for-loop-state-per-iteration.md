---
status: open
priority: high
area: perf
depends: [0011]
touches: [src/statements.cpp#ForStatement]
shares: [src/statements.h, src/render_context.h]
---
# `for` loops rebuild the `loop` map and a scope map on every iteration

**Problem.** Found with the benchmark suite (0011). `Render/mitsuhiko_table` (the
classic 1000 x 10 table) takes about 3.5-4.2 ms in Jinja2C++ and 1.0-1.25 ms in Python
Jinja2: we are about 3.4x slower than the interpreter we are meant to beat. Callgrind
shows where it goes, all inside `ForStatement::RenderLoop` (`src/statements.cpp`):

- Each iteration writes 8-9 keys into the `loop` map (`index`, `index0`, `first`,
  `last`, `revindex`, `revindex0`, `previtem`, `nextitem`, ...), each through
  `loopVar["index"s]`, which builds a `std::string` key and hashes it.
  `robin_hood::Table::operator[]` alone is 20% self time, 30% inclusive.
- `values.EnterScope()` / `ExitScope()` around the body creates a fresh hash map per
  iteration; its `increase_size` and `rehashPowerOfTwo` are another 9%.
- `LoopState` shared-pointer disposal and the loop target assignment (`AssignTo`) add
  about 8% more.

Jinja2 itself computes `loop.index` and friends lazily from a counter.

**Proposal.** Make `loop` a map accessor over `LoopState` that computes each property on
lookup (the counter already lives there) instead of a materialized `InternalValueMap`,
so an iteration only bumps `index0` and swaps the current/next item. Reuse one body
scope across iterations (clear it instead of popping and pushing a new map), keeping
the semantics of `set` inside a loop body. Keep `loop(...)` recursion, `loop.cycle`,
`loop.changed` and `loop` escaping the loop (`set ns.x = loop`) working; forloop_test
and the parity corpus cover them.

**Done when.** `Render/mitsuhiko_table` is no slower than Python Jinja2 in
`bench/run.py` on the same machine, and `Render/for_range`, `Render/for_loop_vars`
improve, with unit and parity tests unchanged.

**Next.** Once the loop machinery is cheap, variable lookup through nested scopes
(`ValueRefExpression::Evaluate`, 7%) and value output (`WriteValue`, 5%) become the
top of the profile; see 0088.
