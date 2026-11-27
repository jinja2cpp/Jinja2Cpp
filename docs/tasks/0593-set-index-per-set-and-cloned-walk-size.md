---
status: open
priority: low
area: robustness
depends: [0587]
touches: [src/internal_value.cpp#CreateValuesAdapter, src/generic_adapters.h, src/value_methods.cpp#SetMethods]
shares: [test/set_index_test.cpp]
---
# Set leftovers after 0587: an index object per set, a cloned walk's size, intersection_update

**Problem.** Left from 0587 (batch 14 D):
- The set adapter makes its `SetIndex` object for every set, temporaries such as `a - b` and `s|list` copies
  included, though most are never looked up. The index cannot simply be made on first use: the adapter's copies share
  it as they share the items (`m_index`), so one made later by a copy would be that copy's alone.
- A cloned set walk (`IndexedListAccessorImpl::Enumerator::Clone`) builds its enumerator from the list and so reads
  the size at the clone, not the one the walk began with, which `CheckStep` compares against.
- `intersection_update` builds the receiver's token table and the new set (O(len(s)) per call); Python's walks the
  smaller side.

**Proposal.** Keep the index (or a slot for it) in the shared storage beside the items, made on the first lookup;
copy `m_maxItems` in `Clone()`; intersect from the smaller side and erase the rest through the index.

**Done when** a set temporary allocates no index, a cloned walk raises at the size the walk began with (a row in
test/set_index_test.cpp), and `intersection_update` with a one-item argument in a loop of 20000 stays linear.
