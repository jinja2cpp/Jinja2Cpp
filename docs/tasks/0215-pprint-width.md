---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/647
priority: low
area: parity
touches: [src/serialize_filters.cpp#PrettyPrinter]
---
# `pprint` does not wrap at Python's width, and other repr gaps

**Problem.** Python's `pprint.pformat` (width 80) breaks a list, tuple or dict whose repr does
not fit on one line into one item per line, indented to the opening bracket, and splits long
strings into implicitly concatenated chunks. `PrettyPrinter` always prints one line:
`{{ range(30)|list|pprint }}` is `[0, 1, ...]` here and `[0,\n 1,\n ...]` in Jinja2
(`filters.pprint_wraps_long`). Found while fixing 0173, which made the one-line form match
Python's repr (None/True/False, tuples, dictsort pairs, string escapes).

Other values `pprint` does not print as Python's repr (`filters.pprint_python_types` pins the
first two): int dict keys are quoted (`{'1': 2}`, the map keys are strings) and a range prints
as a list (`range(0, 3)` in Python); `loop` prints its map (`<LoopContext 1/1>`), a global such
as `range` prints `<callable>` (`<class 'range'>`), an imported template prints a dict
(`<TemplateModule 'lib.j2'>`).

**Proposal.** Port `pprint.PrettyPrinter._format` for the types the engine has: measure each
item's one-line repr, and when the container's does not fit in `80 - indent`, emit one item per
line with the item indent; strings over the width are split at word boundaries as `_pprint_str`
does. Unit rows in `test/filters_test.cpp` and `test/user_callable_test.cpp` that print long
reflected structs will change with it.

**Done when.** `filters.pprint_wraps_long` and `filters.pprint_python_types` match and their divergences lines are deleted.

**Outcome.** `filters.pprint` ports `PrettyPrinter._format` (lists, tuples, dicts, dictsort
pairs and strings) and prints a range as `range(0, 3)`; int dict keys already printed unquoted.
Left over: `loop` printing its map is 0219; builtin globals and imported
templates are 0309 (after 0305) (`filters.pprint_global_and_module`).
