---
status: open
priority: low
area: perf
depends: [0140]
touches: [src/internal_value.h, src/internal_value.cpp]
---
# Is a 24-byte `InternalValue` worth a hand-written union?

**Problem.** 0140 P2 builds a 32 B value from library types: a trivially copyable
`std::variant` (24 B) and a `boost::intrusive_ptr` (8 B). A 24 B value needs the immediate
part in 16 B. `std::variant` cannot do that, because a view `{pointer, 32-bit length}`
already fills 16 B with padding before the index. So it would need a hand-written union.
The prototype measured only 2-3 instructions per operation cheaper than the library layout
(docs/design/0140-internal-value-plan.md §2).

**Proposal.** Decide on P2's numbers. Do it only if cache-sim shows density still matters
(D1 misses on mitsuhiko_table and list-heavy cases), and only with the union encapsulated in
one class with `static_assert`s on layout.

**Done when.** Decided with P2's cache-sim numbers; default is dropped.
