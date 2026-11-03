---
status: open
priority: low
area: parity
depends: [0383]
touches: [src/value_visitors.h#BinaryMathOperation, src/serialize_filters.cpp#ToJson]
shares: [test/parity/cases/filters.py, test/parity/divergences/filters.txt]
---
# A generator still prints, compares and serializes as a list

**Problem.** Found doing 0383. Since 0383 the results of `map`, `select`, `reject`,
`selectattr`, `rejectattr`, `batch`, `slice`, `unique` and `items`, and `reverse` of a
sequence, carry an iterator mark (`IteratorKind`, `ListAdapter::CreateIterator`): they have no
len(), reversed(), slices or list methods, as in Python. The rest of the value model still sees a
list:

- `{{ [1]|select == [1] }}` is True; Python compares a generator by identity (False).
- `{{ [1]|select|tojson }}` gives `[1]`; Python's json raises "Object of type generator is
  not JSON serializable".
- `{{ ([1]|select) + [2] }}` gives `[1, 2]`, `* 2` repeats it; Python raises "unsupported
  operand type(s) for +: 'generator' and 'list'".
- `{{ [1]|select }}`, `|string` and `|pprint` print `[1]`; Python prints
  `<generator object select_or_reject at 0x...>` (`<list_reverseiterator object at 0x...>`
  for `reverse`). The address is never comparable, so this one is a candidate for a deliberate
  divergence or a fixed placeholder address.

Pinned by `filters.lazy_result_equals`, `filters.lazy_result_tojson` and
`filters.lazy_result_concat` in `test/parity/divergences/filters.txt`.

**Proposal.** Check `GetIteratorKind()` where `BinaryMathOperation` takes two lists (== by
identity, + and * a TypeError naming `PythonTypeName`), and in the tojson serializer. Decide the
printed form with Ruslan: Python's repr with an address the corpus cannot match, or a deliberate
line next to 0101. `BinaryMathOperation` is on the render path: measure with `bench/count.py`.

**Done when** the three lines are gone from `test/parity/divergences/filters.txt` and the
printed form is either matched or listed as deliberate.
