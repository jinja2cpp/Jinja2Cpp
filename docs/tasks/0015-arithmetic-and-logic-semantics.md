---
status: open
priority: high
area: parity
depends: [0001, 0014]
touches: [src/value_visitors.h#BinaryMathOperation, src/expression_evaluator.cpp#binary]
shares: [src/value_visitors.h, src/expression_evaluator.cpp, src/internal_value.cpp]
---
# Python arithmetic, comparison and `and`/`or` semantics

**Problem.** Expressions parse but compute different values: `/` does not always yield a
float, `//` and `%` truncate instead of flooring (`-7 // 2` gives `-3`, Python `-4`), `**`
is left-associative, division by zero renders `inf`/`nan` instead of failing, 64-bit
integers overflow or turn into floats, `3 * 'ab'` and `1 + True` give nothing, lists do not
compare by value, `'a' in dict` is false, `0.0` is truthy, `not a == b` binds wrongly, and
`and`/`or` return a bool instead of the deciding operand, which breaks the common
`x and 'yes' or 'no'` idiom. `9223372036854775807 * 2` is signed overflow, undefined behaviour that UBSan reports
(the corpus skips that case as `crash`). Type errors (`'a' + 1`, `1 < 'a'`, calling a number) render
empty instead of raising (31 cases).

**Proposal.** Implement the binary operators against Python's numeric tower: integer
results stay integers, `/` is true division, floor semantics for `//` and `%`, errors for
division by zero and unsupported operand types, right-associative `**`. Make `and`/`or`
return operands. Big integers: decide between an arbitrary-precision type and an
overflow error (recommend the error; record it as a deliberate divergence in
`docs/parity.md`). Precedence fixes go with 0014 if they live in the parser.

**Scheduling.** Precedence is 0014's. The `%` operator on strings (`'%s' % x`) belongs to 0020 but lives in the same `BinaryMathOperation`; leave a clean dispatch point for it.

**Done when.** No line of `test/parity/divergences.txt` names task 0015, and `ctest -R parity` passes
(big-integer cases may stay listed with the deliberate-divergence reason).
