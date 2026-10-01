---
status: done
priority: high
area: parity
depends: [0001, 0014]
touches: [src/value_visitors.h#BinaryMathOperation, src/expression_evaluator.cpp#binary]
shares: [src/value_visitors.h, src/expression_evaluator.cpp, src/internal_value.cpp]
---
# Python arithmetic, comparison and `and`/`or` semantics

**Problem.** Expressions parse but compute different values: `/` does not always yield a
float, `//` and `%` truncate instead of flooring (`-7 // 2` gives `-3`, Python `-4`), `**`
is left-associative, division by zero renders `inf`/`nan` instead of failing (integer
`1 // 0` is worse: undefined behaviour, SIGFPE when run alone, so the corpus skips it as `crash`), 64-bit
integers overflow or turn into floats, `3 * 'ab'` and `1 + True` give nothing, lists do not
compare by value, `'a' in dict` is false, `0.0` is truthy, `not a == b` binds wrongly, and
`and`/`or` return a bool instead of the deciding operand, which breaks the common
`x and 'yes' or 'no'` idiom. `9223372036854775807 * 2` is signed overflow, undefined behaviour that UBSan reports
(the corpus skips that case as `crash`). Type errors (`'a' + 1`, `1 < 'a'`, calling a number) render
empty instead of raising (31 cases).

**Also (found in 0014).** Slices render empty where Python raises: `l[::0]` (ValueError)
and a non-integer bound such as `l[1.5:]` (TypeError); case `subscripts.slice_step_zero`.
The check belongs in `Slice()` in `src/internal_value.cpp`.

**Proposal.** Implement the binary operators against Python's numeric tower: integer
results stay integers, `/` is true division, floor semantics for `//` and `%`, errors for
division by zero and unsupported operand types, right-associative `**`. Make `and`/`or`
return operands. Big integers: decide between an arbitrary-precision type and an
overflow error (recommend the error; record it as a deliberate divergence in
`docs/parity.md`). Precedence fixes go with 0014 if they live in the parser.

**Scheduling.** Precedence is 0014's. The `%` operator on strings (`'%s' % x`) belongs to 0020 but lives in the same `BinaryMathOperation`; leave a clean dispatch point for it.

**Dict equality.** Compare mappings by key set and values, whatever adapter holds them:
`InternalValueMapAdapter::IsEqual` (internal_value.cpp) `dynamic_cast`s to its own
instantiation, so since 0031 a dict literal (`InternalDict`) never equals an
`InternalValueMap`-backed dict (e.g. a `groupby` item) or a `ValuesMap` from the context.
Pinned by `operators.eq_dict` and `operators.eq_dict_order_insensitive`.

**Done when.** No line of `test/parity/divergences/` names task 0015, and `ctest -R parity` passes
(big-integer cases may stay listed with the deliberate-divergence reason).

**Result.** `BinaryMathOperation` follows Python's numeric tower: `bool` is an int, int
results stay ints with overflow-checked `+ - * **`, `/` is true division, `//` and `%`
floor for ints and floats (CPython's `float_divmod`), division by zero raises, ints and
floats compare exactly, and `**` with a negative exponent gives a float. Operands of
unrelated types compare unequal and raise `TypeError` for ordering and arithmetic; lists
and tuples compare lexicographically (a list never equals a tuple), dicts compare by keys
and values whatever adapter backs them (by iteration, so `IsEqual` is untouched), and
`str`/`list` repeat with an int on either side. `and`/`or` short-circuit and return the
deciding operand; `not` and every truth test go through `ConvertToBool`, which no longer
treats non-zero floats as false. `key in dict` tests keys, `x in 5` and `1 in 'abc'`
raise, `l[::0]` and `l[1.5:]` raise, and calling a number, string, list or dict raises
`'int' object is not callable`. The string-by-string subscript overload is gone (the
0037 item, `sequences.join_attribute_mapping`): `{% set a, b %}` assigns the rendered
body to every name explicitly. `str % x` keeps rendering empty through one
`PercentFormat` hook for 0020. Deliberate divergences: integers are int64 and overflow
raises instead of growing (`int_overflow_mul`, `int_big_pow`), a literal beyond int64
becomes a float (`int_big`), and complex results (`(-8) ** 0.5`) are `nan`. Calling an
undefined name still renders empty; that is 0034/0026's.
