---
status: open
priority: medium
area: api
depends: [0270]
touches: [include/jinja2cpp/big_int_backend.h, include/jinja2cpp/template_env.h, src/template_env.cpp, src/template_impl.h, src/render_workspace.h, src/value_visitors.h#BinaryMathOperation, src/filters.cpp#PythonRoundInt, src/big_int.cpp, test/big_int_test.cpp, test/big_int_toy_backend.h, bench/cases/int_arith]
---
# Big-integer backend interface and overflow promotion (0178 phase 3)

**Problem.** With 0270 big values are exact but arithmetic on them raises; users need a way to
plug in their own arithmetic, also into a prebuilt library.

**Proposal** (plan sections 3.2, 3.4, 4). The versioned, size-checked `BigIntBackend` table of 13
noexcept ops (truncating division; floor rules, `**`, rounding and base conversion composed in the
core), `TemplateEnv::SetBigIntBackend`/`GetBigIntBackend`, promotion from 0015's overflow branches as
cold calls with demotion of results that fit int64, and a ~200-line decimal-string test backend so
every CI row tests the interface without Boost. Add a `bench/cases/int_arith` workload.

**Done when.** The 0271 rows of plan section 9 pass on every platform including MSVC, and the perf
gate holds: within ±0.5% Ir and the same allocation counts on every bench case.

Plan: [docs/design/0178-big-integer-plan.md](../design/0178-big-integer-plan.md).
