---
status: done
priority: low
area: parity
depends: [0052]
touches: [src/filters.cpp#DictSort, src/filters.cpp#SequenceAccessor, src/string_converter_filter.cpp#IndentMode]
---
# Filters that accept None where Python raises

**Problem.** Found while fixing 0052 (undefined in filters): the same filters that now fail
on a named undefined still accept `None`, where Python raises because `None` has no such
method or is not iterable. Each is pinned by a corpus case in
`test/parity/divergences/filters.txt`:

- `none|dictsort` renders empty; Python's `value.items()` raises AttributeError
  (`filters.none_dictsort`). The same holds for any non-mapping (`5|dictsort`).
- `none|first` renders empty; Python's `iter(None)` raises TypeError (`filters.none_first`).
  `last`, `min`, `max`, `random` and the other sequence filters share the
  `ConvertToList(..., isConverted)` early return in `SequenceAccessor::Filter`;
  `batch` over None is 0048's item 10 (`filters.batch_none`).
- `none|indent` renders `None`; Python's `s += "\n"` raises TypeError for None and for
  numbers, because `indent` does not call `str()` first (`filters.none_indent`).

**Approach.** Raise Python's TypeError/AttributeError text as a `std::runtime_error` where
the value is neither undefined nor of the expected kind. Check the unit tables
(`test/filters_test.cpp`) for rows that encode the lenient result first.

**Done when.** No line of `test/parity/divergences/` names task 0177 and `ctest` passes.

**Also.** The 4-argument `ConvertToList(val, subscript, isConverted, strict)`
(`src/internal_value.h`, `src/internal_value.cpp`) lost its last caller in 0052 (`join`
now follows Jinja2's attribute paths); delete it with this task's PR.

**Done** in #519. `first`, `last`, `length`/`count`, `random`, `reverse`, `sum`, `min`, `max`,
`sort`, `list`, `groupby`, `map(attribute=)` and `join` raise Python's TypeError text for
None, bools, numbers and callables (`IsNotIterable` in `src/filters.cpp`); `dictsort`
raises the AttributeError for any non-mapping and `indent` for any non-string. `map`, `select`
and `reject` keep Jinja2's `if value:` guard: a falsy input (None, False, 0) gives an empty
result, a truthy one raises. Undefined inputs keep their policy. The 4-argument `ConvertToList` had one more caller
(`testers.cpp`, with an empty subscript) and went with `ToSubscriptedList`, its only user.
