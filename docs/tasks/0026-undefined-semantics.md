---
status: open
priority: medium
area: parity
depends: [0001, 0034]
touches: [src/expression_evaluator.cpp#postfix, src/value_visitors.h#BinaryMathOperation, src/internal_value.cpp#Subscript]
shares: [src/expression_evaluator.cpp, src/value_visitors.h, src/internal_value.cpp, src/filters.cpp, include/jinja2cpp/template_env.h]
---
# Undefined semantics and undefined policies

**Problem.** Jinja2's default `Undefined` prints as empty but raises on attribute, item,
call or arithmetic use; Jinja2C++ renders all of these as empty, so a typo deep in an
expression goes unnoticed. `undefined|length` and `undefined|list` give nothing instead
of `0`/`[]`. There is no way to choose `StrictUndefined` (fail on any use, the usual
choice for code generation, a core Jinja2C++ use case) or `ChainableUndefined` (18
cases).

**Proposal.** Model undefined as a value kind with a policy chosen in `Settings`
(`Default`, `Strict`, `Chainable`, `Debug`), and route attribute/item/call/operator
evaluation on undefined through the policy. Keep the current lenient behaviour reachable
as a policy only if users ask for it.

**Scheduling.** 0034 introduces the undefined value; this task adds the policies on top of it.

**Done when.** No line of `test/parity/divergences.txt` names task 0026, and `ctest -R parity` passes.
