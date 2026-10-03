---
status: open
priority: medium
area: perf
depends: [0011, 0087]
touches: [src/expression_evaluator.cpp, src/expression_evaluator.h, src/internal_value.h]
---
# Expression evaluation is several times slower than Python Jinja2

**Problem.** Found with the benchmark suite (0011). Rendering is where a C++ engine
should win, yet expression-heavy templates lose to Python Jinja2 (Release build, GCC 13,
cloud container; Speedup is Python time / C++ time):

| Benchmark | Jinja2C++ | Python Jinja2 | Speedup |
|---|---:|---:|---:|
| Render/expressions | 365 µs | 60-77 µs | 0.2x |
| Render/dict_ops | 175-199 µs | 67-74 µs | 0.4x |
| Render/for_range | 26-28 µs | 18-20 µs | 0.75x |
| Render/strings | 281-301 µs | 236-298 µs | 0.85-1.0x |

The flat profile of `Render/expressions` has no single hot spot: `malloc`/`free` about
8%, `InternalValue` copy constructor and variant visitation about 7%,
`ValueRefExpression::Evaluate` 5%, `ValueTester::Test` 5%. Python Jinja2 compiles a
template to Python bytecode once and folds constant subexpressions at compile time
(`[1, 2, 3, 5, 8, 13]` is built once); Jinja2C++ walks the expression tree and builds
the list literal on every evaluation.

After 0087 (#357), `Render/mitsuhiko_table` (0.8-0.87x Python) is bound by the same
path: of about 800 instructions per `<td>{{ cell }}</td>`, `{{ cell }}` costs about
450 (`ValueRefExpression::Evaluate` about 220: hashing the name and copying the
`InternalValue`; `WriteValue` the rest), each raw-text write through the virtual
`WriteBuffer` about 70, and fetching the item from a `Value` list about 200
(`ValuesListAdapter::GetItem` converts each `Value` to an `InternalValue`).

**Proposal.** Treat this as a strategic direction rather than one fix; measure each step
with `bench/run.py --baseline`:
1. Constant folding at parse time for literals, tuples/lists/dicts of literals and
   pure operators on them.
2. Fewer `InternalValue` copies on the hot path (evaluate to a reference where the
   value lives in a scope; move instead of copy into function and filter arguments).
3. Name resolution: resolve variable names to scope slots or at least pre-hash them at
   parse time, instead of hashing strings per lookup.
4. Small-object allocation: an arena per render for temporaries.

**Done when.** `Render/expressions`, `Render/dict_ops` and `Render/mitsuhiko_table` are
at least as fast as Python Jinja2 in `bench/run.py` on the same machine.
