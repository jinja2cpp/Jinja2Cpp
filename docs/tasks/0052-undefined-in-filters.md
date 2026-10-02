---
status: open
priority: low
area: parity
depends: [0026]
touches: [src/filters.cpp, src/string_converter_filter.cpp, src/serialize_filters.cpp, src/testers.cpp#IsCallable, src/global_functions.cpp#Range]
shares: [src/undefined.h, src/value_visitors.h]
---
# Undefined in filters, tests and global functions: what 0026 left behind

**Problem.** Task 0026 made undefined values carry what was missing and an undefined
policy (`Settings::undefinedPolicy`), and routed attribute, item, call, operator, print,
truth and iteration use through it. Filters, tests and global functions that look into
their argument in other ways still treat undefined as empty, where Python's `Undefined`
raises (its `__getattr__`, `__int__` and the like fail). `int`, `float` and `attr` already
check (`CheckUndefinedUse` in `src/filters.cpp`). Each gap is pinned by a corpus case in
`test/parity/divergences/undefined.txt`:

- `tojson`, `dictsort`, `format`, `indent`, `wordwrap`, `range()` of undefined render
  instead of raising (`undefined_tojson`, `undefined_dictsort`, `undefined_format`,
  `undefined_indent_wordwrap`, `undefined_range`).
- `map(attribute=...)` (and the other `attribute=` filters) read attributes of undefined
  items without raising (`undefined_map_attribute`).
- `map(nope)` with an undefined filter name renders (`undefined_as_filter_name`).
- `first`/`last` of an empty sequence return a plain undefined; Python's has the hint
  "No first item, sequence was empty." and fails on attribute use (`undefined_first_hint`).
  `MakeUndefinedWithHint` in `src/undefined.h` builds one.
- `undefined is callable` is `True` in Python (`Undefined` defines `__call__`)
  (`undefined_is_callable`).
- With `StrictUndefined`, string filters such as `truncate` accept undefined; Python's
  `str()` of it raises (`strict_truncate`).

**Approach.** For each filter, call `CheckUndefinedUse(value, ...)` where Python touches
the value, choosing the `UndefinedUse` that matches the dunder Python calls. Error paths
that return a plain `InternalValue()` stay lenient on purpose: only undefined values with
info fail.

**Done when.** No line of `test/parity/divergences/` names task 0052, and
`ctest -R parity` passes.
