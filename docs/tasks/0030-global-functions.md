---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/307
priority: medium
area: parity
depends: [0001, 0012]
touches: [src/value_visitors.h#ValueRenderer, src/global_functions.cpp]
shares: [src/value_visitors.h, src/template_impl.h, src/internal_value.cpp, src/internal_value.h, src/expression_evaluator.cpp, src/expression_evaluator.h, src/filters.cpp, CMakeLists.txt]
---
# Global functions: `cycler`, `joiner`, `lipsum`, `range`

**Problem.** Jinja2's default globals are `range`, `dict`, `lipsum`, `cycler`, `joiner`
and `namespace`. Jinja2C++ lacks `cycler`, `joiner` and `lipsum`, and `range` with a
negative step stops one item early (7 cases; `namespace` is in 0021).

**Proposal.** Add the three as builtin callables with state (cycler: `next()`,
`current`, `reset()`; joiner: returns `''` first, then the separator) and fix the range
end condition for negative steps. `lipsum` output is random in Jinja2; match its shape,
not its text.

**Scheduling.** Also owns printing `range(0, 3)` (taken from 0012): give range objects a kind that the value printer can recognise.

**Done when.** No line of `test/parity/divergences/` names task 0030, and `ctest -R parity` passes.

**Outcome.** `range`, `dict`, `cycler`, `joiner` and `lipsum` are `Callable` globals in
`src/global_functions.cpp`; globals set on the environment now take precedence over them.
`range()` returns a list whose accessor keeps its arguments (`RangeInfo`), so it prints as
`range(0, 3)` and `|list` materialises it (tuples too). `lipsum` follows Jinja2's
algorithm with a generator seeded per render. Left over: `cycler.reset()` returns `None`,
which prints empty until 0034; the integer dispatch of `loop.cycle` is 0042.
