---
status: done
priority: low
area: parity
depends: []
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1008
touches: [src/value_methods.cpp#ListIndex, src/value_methods.cpp#SliceIndex]
shares: [test/parity/cases/filters.py, test/parity/divergences/filters.txt]
---
# Tuple and list index bounds, and a bound method's __name__

**Problem.** Found by the 0563 verifier on dictsort pairs, and the same for every tuple and list (Jinja2 3.1.6):
- `(1, 2).index(1, none)` renders `0`: `SliceIndex` reads None as an omitted bound. Python raises "TypeError: slice
  indices must be integers or have an __index__ method" (case `filters.tuple_index_none_bound`).
- `(1, 2).index(1, -9999999999999999999)` fails with "Integer out of range". Python clamps a huge bound as a slice
  bound and renders `0` (case `filters.tuple_index_huge_bound`); `str.index` and friends may share the helper.
- `(1, 2).count.__name__` is empty; Python prints `count` (case `filters.bound_method_name`). Check `__qualname__`
  and `__self__` too.

**Proposal.** Reject a None bound in `index` as Python does (other callers of `SliceIndex` may accept it: check each
against Python), clamp big-int bounds to the length, and give a bound method its `__name__`.

**Done when** the three divergence lines are gone and the cases match in narrow and wide runs.

**Result.** `SliceIndex` reads a bound past int64 as Python's slice index does (negative clamps to 0, positive to the
length), and `list.index`, `tuple.index` and a dict item pair's `index` reject None as Python does; str's methods keep
None as an omitted bound, and a positive huge start is past the end for find, count and startswith. Those methods now parse both bounds before
they answer, and an end before the start matches nothing, not even an empty string (`'abab'.find('', 2, 1)` is -1). `list.insert` reads
its index as an index, so None and a huge integer are errors as in Python (None was accepted as 0). A bound built-in
method answers `__name__` and `__qualname__` (`tuple.count`) from the name it keeps, with no allocation when binding.
Cases `filters.tuple_index_*`, `filters.bound_method_*`, `filters.list_index_*`, `filters.str_find_*` and
`filters.list_insert_*`, `filters.str_*_past_end_bad_end`, `filters.str_*_empty_*inverted`. Format spec gaps the
verifier found on master are 0585.
