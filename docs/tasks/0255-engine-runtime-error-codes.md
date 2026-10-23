---
status: done
priority: medium
area: robustness
depends: [0253, 0254]
touches: [src/engine_errors.h, src/error_boundary.h, src/filters.cpp, src/expression_parser.cpp, src/serialize_filters.cpp, src/internal_value.cpp, src/value_visitors.h, test/errors_test.cpp, test/filters_test.cpp, .git-blame-ignore-revs, REVIEW.md]
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

**Result (PR #612).** `TemplateRuntimeError` lives in src/engine_errors.h with the engine's other
error types, not in error_handling.h. Commit A (9e2f697, in `.git-blame-ignore-revs`) rewrote 139
sites. The integer overflow part (`ThrowIntegerOverflow` to `IntegerOutOfRange`) is left to
0270, whose PR changes that function; until it lands, `{{ 9223372036854775807 + 1 }}` reports 17.
The sequence cap already threw `ResourceLimitError` (0262). The parse-time catch in
`ParseIfExpression` now returns a parse error instead of printing to `std::cout`.
`std::logic_error` invariant checks stay for 0265.
