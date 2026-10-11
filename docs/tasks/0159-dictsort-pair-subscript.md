---
status: open
priority: low
area: parity
depends: []
touches: [src/internal_value.cpp, src/value_visitors.h]
---
# The pairs of dictsort read as 2-tuples

**Problem.** `{% set p = {'ab': 1}|dictsort|first %}{{ p[0] }}|{{ p[1] }}|{{ p|length }}`
renders `ab|1|2` in Jinja2: dictsort returns `(key, value)` tuples. Jinja2C++ renders `||`
with nothing for the length. dictsort's items are `KeyValuePair` values, and
`SubscriptionVisitor` (internal_value.cpp) and `ConvertToList` have no overload for them,
so a subscript gives undefined. Iterating and unpacking a pair work (`for k, v in
d|dictsort`), and the pairs of `dict.items()` are real tuples, so only direct reads of a
dictsort pair fail. Found while checking the 0140 P0a seam.

**Proposal.** Either make dictsort return 2-tuples as `items()` does, or give
`KeyValuePair` the read API of a 2-tuple (subscript 0/1 and -1/-2, length 2, iteration,
`list(p)`). The first is simpler and matches Python's type (`p is sequence`, `p|list`);
measure dictsort before choosing, since the pair is cheaper to build.

**Done when** the corpus case `filters.dictsort_pair_subscript` matches and its line in
`test/parity/divergences/filters.txt` is deleted.
