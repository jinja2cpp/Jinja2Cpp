---
status: open
priority: low
area: parity
depends: []
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
