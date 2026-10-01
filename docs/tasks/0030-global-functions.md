---
status: open
priority: medium
area: parity
depends: [0001]
touches: [src/template_impl.h, src/internal_value.cpp, src/expression_evaluator.cpp]
---
# Global functions: `cycler`, `joiner`, `lipsum`, `range`

**Problem.** Jinja2's default globals are `range`, `dict`, `lipsum`, `cycler`, `joiner`
and `namespace`. Jinja2C++ lacks `cycler`, `joiner` and `lipsum`, and `range` with a
negative step stops one item early (7 cases; `namespace` is in 0021).

**Proposal.** Add the three as builtin callables with state (cycler: `next()`,
`current`, `reset()`; joiner: returns `''` first, then the separator) and fix the range
end condition for negative steps. `lipsum` output is random in Jinja2; match its shape,
not its text.

**Done when.** No line of `test/parity/divergences.txt` names task 0030, and `ctest -R parity` passes.
