---
status: done
priority: medium
area: robustness
depends: []
touches: [src/internal_value.cpp, src/filters.cpp, src/value_visitors.h]
---
# Strings from generator items outlive the item under autoescape `join`

**Problem.** Found by the verifier of the 0111 fix (2026-10-05). With `l` a generator made by
`MakeGenericList` over dicts, `{% autoescape true %}{{ l | join('<', attribute='s') }}`
prints `b&lt;c&lt;c` where Python prints `d&lt;b&lt;c`. Valgrind on the Debug build reports
an invalid read in `ValueRendererBase::AppendString`: the string is a `string_view` into
a generator item's `ValuesMap` that was already freed. Autoescape `join` collects the
items first, and by then the generator has moved on and released the earlier item.
`l | map(attribute='s') | join('<')` and `l | list | join('<', attribute='s')` show the same
read and do not go through the 0111 code, so master has it; 0111 only made the
`join(attribute=)` form reachable (it used to hang). ASan and UBSan in RelWithDebInfo do
not catch it and print the right output; only valgrind on Debug does.

Repro driver: a generator over three dicts `{s: 'd'}`, `{s: 'b'}`, `{s: 'c'}` rendered as above.

**Done when** subscripting a borrowed item keeps its parent alive (`SetParentData`, as
`SequenceAccessor` does with `WithParent`) or the value is copied, a unit test in
`test/containers_api_test.cpp` covers the three templates, and valgrind is clean on them.

**Done** in PR #390 (0115's first PR): a generator's item arrives as a value that owns its
`ValuesMap` (`BySharedVal`), and items lent from such a map now copy their strings instead of
viewing into it. `ContainersApiTest.GeneratedItemStringsOutliveTheItems` covers the three
templates; valgrind on the Debug build reports the invalid read on master and none with the fix.
