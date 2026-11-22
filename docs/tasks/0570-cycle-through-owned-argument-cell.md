---
status: open
priority: medium
area: robustness
depends: [0336, 0402]
touches: [src/borrowed_cells.cpp, src/borrowed_cells.h, test/borrowed_cells_test.cpp, test/binding/conformance.h]
---
# A cycle through the cell of a module call's owned argument keeps it for the module's life

**Problem.** Found by the 0402 verifier (round 2). A macro that writes a container into itself
(`{% macro selfref(d, f) %}{% set _ = d.update(self=d, f=f) %}{% endmacro %}`), called with a fresh
owned argument each time, leaves that argument's cell, and everything it holds, in the module's
registry until the module goes: the item `self` is an adapter of the same data whose owning link
counts on the cell, so `BorrowedCell::IsUnheld` (`use_count() == 2`) never holds, and nothing walks the
cell, which is live, not frozen. `DropUnheld` then scans one more cell at every call's end, so the
cost grows with the calls (the verifier measured 0.12 s, 0.41 s, 1.53 s for 1000, 2000, 4000 calls).

Repro (master, a host `ValuesMap`): 50 calls of `selfref` with
`ValuesList{ Value(ValuesMap{{"a", ValuesList{0}}}), Value(counted) }`, where `counted` is a
`UserCallable` capturing a `shared_ptr` token: the token's `use_count()` is 51 after the loop.
Since 0402 an owned JSON document (`Reflect(std::make_shared<J>(...))`, a moved-in document) behaves
the same; before 0402 its cell was call-scoped and went to the collector at the call's end.

**Proposal.** At a call's end, offer the module's cells of data that cannot be lent again (the source's
accessor has no owner outside the registry's cells) to the cycle collector as candidates with the
table's reference discounted, or freeze such a cell and note it (`m_callFrozen`) when the trial
count says only its own items hold it. Keep the 0336 rule: a cell whose data the host can lend again
stays live. Model the candidate step in CellProtocol.tla's `DropUnheld` (a cycle is not modelled yet).

**Done when** the repro's token count stays bounded for host maps and owned JSON documents, with a
test for each (the JSON one as a `selfref` row of `ModuleCallsOfAnOwnedDocument`'s expiry check).
