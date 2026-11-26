---
status: open
priority: low
area: robustness
depends: [0561]
touches: [src/set_index.h, src/value_methods.cpp#SetMethods]
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
