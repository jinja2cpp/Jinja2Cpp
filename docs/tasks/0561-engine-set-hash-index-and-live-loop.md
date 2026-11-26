---
status: in-progress
priority: low
area: robustness
depends: [0545]
touches: [src/value_methods.cpp#SetMethods, src/statements.cpp, src/generic_adapters.h]
shares: [test/parity/cases/methods.py, test/parity/divergences/methods.txt]
---
# The engine's set has no hash index, and a loop over a set changed in its body goes on

**Problem.** 0545 gave the engine's set (`d.keys() - []`) Python's methods. The set is a list adapter
with no index, so `add`, `remove`, `discard` and `in` scan it (`ItemEquals`, O(n) each). A loop over
a set that its body grows, `{% for x in s %}{% do s.add(x + 1) %}{% endfor %}`, reads the set by
index and goes on, where Python raises `Set changed size during iteration` at the next step
(`methods.set_add_during_loop`, `methods.set_remove_during_loop`, deliberate lines of 0545). With
the scan, such a loop slows down quadratically until `maxSequenceSize` or `maxRenderSteps` stops it;
neither is set by default, so an untrusted template can spend O(n^2) time on it.

**Proposal.** Keep a hash index beside the set's items (the token hashing that dicts use), and give
the loop over a set the size check a dict walk has (0538): a size change between steps raises
Python's error.

**Done when** the two divergence lines are gone and a bench or limits test shows `add` in a loop
is linear.

**As built.** `src/set_index.h` holds `SetIndex`, a cache beside the set's items: hash token (`MakeDictKey`) to
position, reached through the new internal `IListAccessor::GetSetIndex()` (null for every list but the engine's set,
whose adapter makes one). A lookup verifies the hit (position in bounds, `ItemEquals` or the `in` equality) and
rebuilds when the index size differs from the items' (lazily on first use, after the collector's `Clear()`, after
`*_update`/`clear`, which `Reset()` it). `add`, `remove`, `discard`, `pop`, `clear`, `in` use it; `add` in a loop is
linear (`SetIndexLinearTest`). An erase renumbers later positions (insertion order stays), so `remove`, `discard` and
`pop` find in O(1) but still cost O(n) for the list erase and the renumbering. Entries are charged to
`maxRenderMemory` (`ChargeRenderMemory`, tracked per index, given back at most as charged on erase, `Reset` and
destruction). The set's enumerator checks its size at each step (`CheckStep`, `generic_adapters.h`) and raises
`Set changed size during iteration`; lists keep the live size. The two divergence lines are deleted; rows are in
`test/set_index_test.cpp` and `test/limits_test.cpp`.
