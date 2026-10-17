---
status: open
priority: low
area: parity
touches: [src/serialize_filters.cpp#PrettyPrinter]
---
# `pprint` does not wrap at Python's width

**Problem.** Python's `pprint.pformat` (width 80) breaks a list, tuple or dict whose repr does
not fit on one line into one item per line, indented to the opening bracket, and splits long
strings into implicitly concatenated chunks. `PrettyPrinter` always prints one line:
`{{ range(30)|list|pprint }}` is `[0, 1, ...]` here and `[0,\n 1,\n ...]` in Jinja2
(`filters.pprint_wraps_long`). Found while fixing 0173, which made the one-line form match
Python's repr (None/True/False, tuples, dictsort pairs, string escapes).

**Proposal.** Port `pprint.PrettyPrinter._format` for the types the engine has: measure each
item's one-line repr, and when the container's does not fit in `80 - indent`, emit one item per
line with the item indent; strings over the width are split at word boundaries as `_pprint_str`
does. Unit rows in `test/filters_test.cpp` and `test/user_callable_test.cpp` that print long
reflected structs will change with it.

**Done when.** `filters.pprint_wraps_long` matches and its divergences line is deleted.
