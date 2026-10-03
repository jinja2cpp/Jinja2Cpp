---
status: done
priority: medium
area: perf
depends: [0011, 0087]
touches: [src/expression_evaluator.cpp, src/expression_evaluator.h, src/internal_value.h, src/internal_value.cpp, src/testers.cpp, src/value_visitors.h, src/out_stream.h, src/render_context.h, src/statements.cpp#ForStatement]
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

**Plan (approved by Ruslan 2026-10-03).** The cause behind the flat profile is the value
model: Jinja semantics need values that are safe to copy, and `InternalValue` is costly to
copy (136 bytes, two variants with `m_parentData` almost always empty; list and map
adapters keep their accessor in a `std::function` that heap-clones on every copy).
Variant copy/move/destroy is 14-23% of the three benchmarks, malloc/free another 9-18%.
Stages, each measured with `bench/count.py`:

All stages landed in PR #362, one commit each:

- S1 `x in [literal]`: items of a literal of scalar constants built once in the node;
  other lists compared in place or through one enumerator.
- S2 numbers call the `BinaryMathOperation` overload directly (return prvalues: an
  `InternalValue&` out-parameter version was slower than `Apply2`).
- S3 `m_parentData` behind `shared_ptr<const>` (136 to 88 bytes): -4% to -15% everywhere.
- S4 `OutStream` holds a writer pointer; narrow output appends buffers directly.
- S5 `EvaluateRef`: variables and constants read in place for `{{ x }}`, operands (the
  left one only when the right one is pure) and the root of `x.attr`/`x['const']`.
  Contract: scope storage stays node-stable during an expression (robin_hood node map
  and `std::deque` of scopes). Guards pinned by `methods.operand_changed_by_call*`.
- S6 list/map accessors behind `shared_ptr`; the generator list clones on copy. Copies
  of a computed list are now the same object, as in Python (`tests.sameas_computed_list`).
- S7 a pair assigned to `for k, v` goes straight into two cached slots.
- S8 names hashed once at parse time (transparent hasher); worth under 1%.

Rejected: slot-resolved names (dynamic scoping, 0038), a per-render arena (allocator
threading, Apple `<memory_resource>`), general constant folding (no payoff measured,
needs error deferral and copy-on-escape). After all stages: re-measure and look for
further wins.

Result (instructions per render with `count.py`, wall clock with `run.py`, same machine):

| Benchmark | master 0a3477c | PR #362 | vs Python before | vs Python after |
|---|---:|---:|---:|---:|
| Render/expressions | 3,396,897 | 895,053 | 0.21x | 1.17x |
| Render/dict_ops | 1,623,385 | 706,609 | 0.40x | 1.33x |
| Render/mitsuhiko_table | 16,731,577 | 12,344,461 | 0.98x | 1.40x |

Every Render benchmark is 7-74% cheaper and every Load benchmark slightly cheaper.
Further ideas from the new profile are in 0100.
