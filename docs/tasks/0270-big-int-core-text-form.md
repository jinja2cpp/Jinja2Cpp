---
status: open
priority: medium
area: parity
depends: [0269]
touches: [src/internal_value.h, src/internal_value.cpp, src/value_visitors.h, src/lexer.cpp, src/lexer.h, src/filters.cpp, src/python_format.cpp, src/serialize_filters.cpp, src/out_stream.cpp, src/testers.cpp, src/value_methods.cpp, src/expression_evaluator.cpp, src/expression_evaluator.h, src/global_functions.cpp, src/big_int.h, src/big_int.cpp, test/big_int_test.cpp, test/parity/divergences/literals.txt, test/parity/divergences/filters.txt]
shares: [test/parity/cases/]
---
# Big integers in the core, text form (0178 phase 2)

**Problem.** After 0269 the engine still cannot carry a big integer through a render, and a
literal beyond int64 becomes a float.

**Proposal** (plan sections 3.3, 5, 6). `ValueKind::BigInt` in `InternalValue` (stays 32 B, one
boxed handle, allocated like `StringObject`), a deleted catch-all in `BaseVisitor` so every unhandled
site is a compile error, and every "without backend" cell of section 6: printing, comparison,
sort/unique, hashing by decimal spelling, `tojson`, `%d`/`%x`, `float`, subscripts and `range`.
Integer arithmetic on a BigInt raises OverflowError (as 0015). Limits: 2^20 bits
(`ResourceLimitExceeded`) and the 4300-digit decimal limit (ValueError).

**Done when.** `literals.int_big` and `filters.int_out_of_range` leave the divergences, the corpus
cases listed in plan section 9 are added, the sanitizer configuration is clean and the perf gate
(section 4) shows no change for int64 templates.

Plan: [docs/design/0178-big-integer-plan.md](../design/0178-big-integer-plan.md).
