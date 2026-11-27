---
status: in-progress
priority: low
area: robustness
depends: [0561]
touches: [src/set_index.h, src/value_methods.cpp#SetMethods, src/internal_value.cpp#CreateValuesAdapter, src/internal_value.h, src/testers.cpp]
shares: [test/set_index_test.cpp]
---
# The engine's set still spends O(n) per remove, pop and update

**Problem.** Leftovers of 0561 (its implementer's notes). The set's hash index (`SetIndex`) finds an item in O(1), but
`remove`, `discard` and `pop` erase from the item list and renumber the later positions, O(n) each: a loop that pops or
discards every item is quadratic (with a small constant). `update` and the other `*_update` methods build their own
token table (`SetEntries`) and reset the index, so `{% for x in r %}{% do s.update([x]) %}{% endfor %}` is quadratic
too. An item whose token matches another's that `ItemEquals` rejects (a NaN) leaves the index smaller than the items, so
each lookup rebuilds it: `{% do s.add(nan) %}` in a loop is quadratic. The set adapter also allocates its `SetIndex`
object for every set, temporaries such as `a - b` included, and a cloned set walk reads the size at the clone. A lazy
filter over a set (`s|map('string')`, `select`, `reject`, `unique`, `batch`) reads the items when it starts, so a loop
over it that grows the set goes on where Python raises `Set changed size during iteration`, and
`{% set it = s|map('string') %}{% set _ = s.add(9) %}{{ it|list }}` prints `[1, 2, 3]` where Python prints
`['1', '2', '3', '9']` (verifier r2 of batch 13 D).

**Proposal.** Positions as sequence ids with a tombstone count, compacted when the tombstones pass half; `*_update`
through the index; NaN items kept by identity (Python's set holds each NaN object once) or counted beside the index;
the index object made on first use.

**Done when** pop/discard/update in a loop of 20000 and repeated NaN adds stay linear in a limits test, the lazy filter
rows match Python (or are listed as deliberate in test/parity/divergences/methods.txt), and the 0561 rows still pass.

**Resolution (batch 14 D).** An erase (`remove`, `discard`, `pop`, `difference_update`) empties its slot and marks it
erased in the index (`SetIndex::Erase`), with no renumbering; the slots close up, positions with them, once the erased
ones pass half or any read other than the set's own methods asks for the items (the adapter's `Values()`,
`GetMutableItems`). Set methods and `in` use the slots directly (`IListAccessor::GetSetSlots`); `len()`, the walk's
size check and `IsMutable` (`HasMutableItems`) count without closing up. `pop()` keeps Python's first item and finds
it from a head cursor. `update`, `difference_update` and `symmetric_difference_update` go through the index;
`intersection_update` still makes the new set (O(len)). A hit of an equal token that `ItemEquals` rejects (a NaN, a
tuple holding one) is the item: the set holds one NaN, as the engine's dict holds one NaN key (two NaN objects Python keeps apart are one item: deliberate line `methods.set_distinct_nans`), and an unhashable or
shadowed item counts beside the index instead of making every lookup rebuild it. Debug timings for 20000 steps:
pop 7.0 s -> 0.2 s, discard 7.7 s -> 0.27 s, update over 300 s -> 0.15 s, repeated NaN add 95 s -> 0.1 s
(`SetIndexLinearTest.ChangesInALoopAreLinear`). The lazy filter rows are listed as deliberate in
`divergences/methods.txt`: map, select, reject, unique and batch are eager for lists as well (0101, 0206). The
index object per set and the cloned walk's size moved to 0593.
