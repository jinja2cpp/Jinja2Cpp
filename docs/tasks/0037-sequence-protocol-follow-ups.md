---
status: open
priority: medium
area: parity
depends: [0016]
touches: [src/value_visitors.h#BinaryMathOperation, src/internal_value.cpp#Subscript]
shares: [src/filters.cpp, src/testers.cpp, src/statements.cpp, test/statements_tets.cpp]
---
# Sequence protocol follow-ups from 0016

**Problem.** Task 0016 (PR #297) made strings and mappings iterable in every filter, as
in Python. That reached code paths that were dead before, and six behaviours still
differ from Jinja2 (cases in `test/parity/cases/sequences.py`), plus two argument
errors that render instead of raising:

- `sort_utf8`, `min_max_utf8`: case-insensitive comparison of narrow strings uses
  `boost::algorithm::is_iless` on `char` (`src/value_visitors.h`, `BinaryMathOperation`),
  which compares signed bytes, so `'é'` sorts before `' '` and `'a'`. Python compares
  code points after `str.lower()`. Wide strings are already right; case-sensitive `<` is
  right because `std::string` compares unsigned.
- `join_attribute_mapping`, `groupby_string`: `SubscriptionVisitor` has an overload
  `(std::basic_string value, std::basic_string field)` that returns the string itself, so
  `'a'['x']` is `'a'` when both are `std::string` (context strings arrive as views and
  are not affected). `m|join(attribute='x')` then joins the keys and
  `s|groupby('x')` groups by characters. Python gives an undefined value and raises in
  `groupby`. Deleting the overload breaks only `SetBlockStatement.MoreVars*`
  (`{% set a, b %}...{% endset %}`, a C++-only extension), which relies on it in
  `src/statements.cpp`.
- `sum_string`: `s|sum` returns the string; Python raises `TypeError` (`0 + 'h'`).
- `slice_zero`, `wordwrap_zero_width`: `slice(0)` renders nothing and `wordwrap(0)`
  returns the input (both guard what was a division by zero or an endless loop);
  Python raises `ZeroDivisionError` and `ValueError: invalid width`.
- `mapping_is_sequence`: `m is sequence` is false; Python says true for a dict. A
  reflected struct is also a `MapAdapter` here and must stay false, so the value model
  needs a way to tell a mapping from an object.

**Proposal.** Compare characters as code points in the case-insensitive path (lower-case
ASCII, compare everything else by code point, as `SplitCodePoints` defines a character).
Remove the string-by-string subscript overload and make the multi-target `set` block
assign the rendered body explicitly. Make `sum` raise when it adds a string to a number,
once 0015 defines how arithmetic errors surface. Report the two argument errors as render errors. Add a mapping-versus-object flag to
`MapAdapter` (or its accessor) and use it in `is sequence`.

**Done when.** No line of `test/parity/divergences/` names task 0037, and
`ctest` passes.

**Progress.** 0015 removed the string-by-string subscript overload and made the
multi-target `set` block assign the body explicitly, which fixed `join_attribute_mapping`;
`groupby_string` still renders (groupby does not yet raise on an undefined key).
