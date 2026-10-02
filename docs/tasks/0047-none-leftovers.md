---
status: open
priority: medium
area: parity
depends: [0034]
touches: [src/internal_value.cpp#GenericMapAdapter, src/value_visitors.h#ValueRenderer, src/string_converter_filter.cpp]
shares: [src/value_visitors.h, src/internal_value.cpp]
---
# None and undefined: what 0034 left behind

**Problem.** 0034 split `None` (`EmptyValue`) from undefined (`UndefinedValue`, the default
`InternalValue`). Three gaps remain (item 3 was fixed by 0019):

1. **JSON `null` and reflected fields.** `GenericMapAdapter::GetItem`
   (`src/internal_value.cpp`) turns an empty `Value` into undefined, because reflected
   struct fields that return `Value()` mean "absent" (`map(attribute=..., default=...)` in
   `filters_test.cpp` relies on it). So `{"x": null}` from a JSON binding gives
   `json.x is defined` false and prints empty, where Python gives `True` and `None`. A JSON
   `null` inside an array is already `None`. Fixing it needs the map accessor to tell
   "present but null" from "absent" (for example through `HasValue`), without breaking the
   reflected-field convention.
   Since 0026, `MakeUndefined` (`src/undefined.cpp`) keeps such a key a plain, lenient
   undefined (no `UndefinedInfo`) through the same `HasValue` check, so `json.x.y` renders
   empty as Python's `None.y` does; drop that guard once `GetItem` returns `None` for it.
2. **Repr of undefined.** Python prints undefined inside a container as `Undefined`
   (`{{ [nope, 1] }}` gives `[Undefined, 1]`). The renderer keeps `None` there, because
   item 1 would otherwise print `{'x': Undefined}` for a JSON object holding `null`. Do
   item 1 first, then switch the repr in `ValueRendererBase`.
3. **String filters on None.** Python applies `str()` first, so `none|upper` is `NONE`,
   `none|replace('o', '0')` is `N0ne`, `none|center(6)` is ` None `. The string
   converters in `string_converter_filter.cpp` treat `None` like undefined and give empty.
   `none|string` and printing already give `None`. Done in 0019: the string filters apply
   `str()` first, and `undefined.none_string_filters` matches.

Cases: `undefined.undefined_in_list`, `undefined.none_string_filters`.

**Done when.** No line of `test/parity/divergences/` names task 0047, and `ctest -R parity` passes.
