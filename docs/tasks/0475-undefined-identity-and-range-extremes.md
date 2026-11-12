---
status: open
priority: low
area: parity
depends: [0413, 0420]
touches: [src/filters.cpp#IsLessKey, src/internal_value.cpp#SliceVisitor, src/value_visitors.h#BinaryMathOperation, src/value_methods.cpp#FormatItem]
shares: [test/parity/divergences/filters.txt, test/parity/divergences/operators.txt]
---
# Leftovers of 0413 and 0420: undefined identity, range extremes, repeat messages

**Problem.** Found by the verifier of the 0413/0420 PR (Jinja2 3.1.6):

1. `{% set u = zzz %}{{ [u, u]|sort|list|length }}` under `StrictUndefined` prints `2` in Python:
   list comparison tests identity before `==`, so the same undefined object is never compared.
   Jinja2C++ raises `UndefinedError` (an undefined value has no identity). Distinct undefineds
   match Python. Pinned by `filters.sort_same_undefined_strict`.
2. Range slices at the int64 edge: `range(10)[::-9223372036854775808]` prints step
   `-9223372036854775807` (GetIndices clamps the step); Python keeps `-9223372036854775808`.
   A slice whose bounds leave int64 is a list of items, not a range (ranges are int64, 0178).
3. Messages only: `d.items() * 'a'` names `'str'` where Python names `'dict_items'`;
   `Markup * x` for a non-int `x` says `can't multiply sequence` where Python says
   `'str' object cannot be interpreted as an integer`; `{'a': 1}.items() * 2` renders a list
   where Python raises (dict views are lists here).
4. Found by Codex on #846: `{{ '{0[start]}'.format(range(3)) }}` renders `0` outside the sandbox;
   Python's `str.format` subscripts without Jinja2's attribute fallback and raises `TypeError:
   range indices must be integers or slices, not str` (a namedtuple's `{0[field]}` likewise).
   `FormatItem` (value_methods.cpp) reaches the attribute fallback through `GetItem`. Pinned by
   `operators.format_field_range_attribute_item`. Fix: outside the sandbox, a list with a string
   key raises `<type> indices must be integers or slices, not str` in `FormatItem`.

**Proposal.** 4: as described in item 4. 1: compare the identity of an undefined's origin (the same `set` binding) before
the `==` check, or record it deliberate. 2: carry the unclamped step when the range slice is
taken. 3: match the messages if dict views gain a type of their own.

**Done when** item 1's divergence line is gone or recorded deliberate, and items 2-3 are fixed or
recorded deliberate.
