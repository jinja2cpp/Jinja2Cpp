---
status: open
priority: medium
area: robustness
depends: [0253, 0254]
touches: [src/error_handling.h, src/filters.cpp, src/expression_parser.cpp, src/value_visitors.h, test/errors_test.cpp]
shares: [src/serialize_filters.cpp, src/statements.cpp, src/python_format.cpp, src/internal_value.cpp, src/string_converter_filter.cpp, src/testers.cpp, src/global_functions.cpp, src/expression_evaluator.cpp, src/value_methods.cpp]
---
# Error contract P3: the engine's own runtime errors get their own codes

**Problem.** The engine raises Python's `TypeError`/`ValueError` as plain `std::runtime_error`
(about 130 sites), so the boundary cannot tell them from a host exception, and the engine's own
`catch (const std::runtime_error&)` sites (filters.cpp, expression_parser.cpp) swallow host
exceptions passing through. Plan: docs/design/0250-error-contract-plan.md §3e.

**Scope.** Commit A: scripted rewrite of `throw std::runtime_error(` to
`detail::TemplateRuntimeError` (derives from `std::runtime_error`), listed in
`.git-blame-ignore-revs`. Commit B: integer overflow to `IntegerOutOfRange`, the sequence cap to
`ResourceLimitExceeded`; narrow the internal catches to `TemplateRuntimeError`; replace the
`std::cout` print on a parse failure in expression_parser.cpp with a parse error.

Touches parity-hot files: land at a wave boundary with the merge steward. Coordinate with 0263
(per-operation size checks), which raises `ResourceLimitExceeded` from more sites.

**Tests.** errors_test.cpp: `{{ 1 + 'a' }}` gives 17; `{{ 9223372036854775807 + 1 }}` gives 16;
`{{ ('x' * 100000000)|length }}` gives 18; update the rows expecting 1; re-check
recursion_limits_test.cpp and containers_api_test.cpp. The parity corpus is unaffected.

**Done when.** No engine-raised error reports `UnexpectedException`.
