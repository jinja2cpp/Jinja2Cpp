---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/846
priority: low
area: parity
depends: [0401]
touches: [src/internal_value.cpp#SubscriptionVisitor, src/internal_value.cpp#SliceVisitor, src/value_visitors.h#BinaryMathOperation, src/filters.cpp#CompareForOrder]
shares: [test/parity/divergences/operators.txt, test/parity/divergences/globals.txt]
---
# Range and engine-object leftovers of 0401

**Problem.** Found by the verifier of 0401 (Jinja2 3.1.6):

1. `{{ range(3)[1:] }}` prints `[1, 2]`; Python's slice of a range is `range(1, 3)`. Pin
   `operators.range_slice_is_range`.
2. `range(1, 9, 2).start`, `.stop`, `|attr('step')` are undefined; Python gives 1, 9, 2. Pin
   `operators.range_attributes`.
3. `joiner()['operator()']` returns the joiner's call; Python's joiner has no items. The engine's
   call slot `operator()` should not be visible to subscripts of engine objects. Pin
   `globals.joiner_call_slot_hidden`.
4. Messages only (the corpus compares the error class, so no pin): `[range(3), range(2)]|max`
   says `'<' not supported` where Python says `'>'`; `[1] * range(3)` and `range(3) * 'a'` say
   `unsupported operand type(s) for *` where Python says `can't multiply sequence by non-int of
   type 'range'`.
5. Not pinned: `loop.__call__` prints nothing; Python prints the bound method.

**Proposal.** 1: a slice of a list with `GetRangeInfo()` makes `CreateRange` with the sliced
bounds. 2: `start`/`stop`/`step` read from `RangeInfo` in the list's attribute lookup. 3: hide
`operator()` from string subscripts of engine objects; keep the call path
(`expression_evaluator.cpp`, `Subscript(fnVal, "operator()")`) on a separate accessor. 4: match
Python's messages in `max`/`min` and `RepeatList`.

**Done when** the three 0420 lines are gone from `test/parity/divergences/`.

**Resolution (parity batch 5, group C).** 1: `SliceVisitor` makes a slice of a range the range
`range(start + i*step, start + j*step, step*k)` from the clipped slice indices, as CPython does;
bounds that leave int64 fall back to the list of items (ranges are int64, 0178). 2:
`SubscriptionVisitor::SubscriptField` answers `start`, `stop` and `step` of a range, so `.start`,
`['start']` and `|attr('start')` agree. 3: `x['operator()']` (and `|attr('operator()')`) of an
engine object other than a namespace is undefined; the call path still reads the slot through
`Subscript(fn, "operator()")`. 4: a failed `seq * x` names the operand as CPython's sequence
repeat does, and `max` compares `item > best`, naming `'>'` and both types. Pinned by
`operators.range_slice_bounds`, `operators.range_item_attributes`, `globals.engine_call_slot_hidden`,
`globals.namespace_call_key_item`, and the messages by `ExpressionsTest.RepeatAndOrderingMessages`.
5 is filed as 0474; the int64-edge and message leftovers the verifier found are 0475.
