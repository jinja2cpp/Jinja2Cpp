---
status: done
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

**Note from 0034.** Undefined is `UndefinedValue`, the default alternative of
`InternalValueData`, so every error path that returns `InternalValue()` is an anonymous
undefined too. A strict policy must tell a named lookup miss (`ValueRefExpression`,
`Subscript`) from those, for example by giving `UndefinedValue` a shared hint pointer, or
make those paths raise as Python does.

**Done when.** No line of `test/parity/divergences/` names task 0026, and `ctest -R parity` passes.

**Outcome ([PR #325](https://github.com/jinja2cpp/Jinja2Cpp/pull/325)).** `Settings::undefinedPolicy` (`UndefinedPolicy::Default`, `Strict`,
`Chainable`, `Debug`) selects the behaviour; a failed use reports
`ErrorCode::UndefinedError` with Python's message in `ExtraParams[0]`. `UndefinedValue`
carries a shared `UndefinedInfo` (`src/undefined.h`): the missing name, the type of the
object it was missing from, an optional hint and the policy, so a value set from a
missing name fails where it is used, as Python's `Undefined` object does. Only named
misses get info: variable lookups, attribute and item lookups, missing macro arguments.
Error paths that return a plain `InternalValue()` stay lenient (print empty, fail no use).
The default policy is Python's `Undefined`, so templates that read an attribute of a
missing variable now fail where they rendered empty; there is no lenient policy, since
nobody asked for one. Deliberate divergences: a key that a host map has but that reads as
undefined (JSON `null` from a binding, an empty reflected field) stays lenient until 0047
tells null from absent. Leftovers in filters, tests and `range()` are task 0052.

