---
status: open
priority: medium
area: robustness
depends: [0336, 0402]
touches: [src/internal_value.cpp, src/borrowed_cells.cpp, test/binding/conformance.h]
---
# A template module keeps the cell of every nested item it wrote, one per call

**Problem.** Found by the 0402 verifier. A nested container read through its parent (`d.a`, a loop
item, `x[0]`) is lent through an aliasing accessor over a holder (`WithOwner` in internal_value.cpp).
`AccessorHasOtherOwners` treats every aliasing accessor as one the host may lend again, so
`BorrowedCell::IsUnheld` never reports such a cell as droppable: in a template module each write to a
nested item of a fresh argument leaves a cell, and the cell's source keeps the argument's data alive,
until the module goes. `DropUnheld` walks all of them at every call's end, so a macro called per request
gets slower with every call (the verifier measured 1.5 s, 6.8 s, 24 s for 2000, 4000, 8000 calls).

Where it shows today: JSON traits without an `Identity` hook (the toy traits of the conformance suite)
and, by reading, nested items of a host `ValuesList`/`ValuesMap` or reflected struct passed as a module
call's argument. 0402 avoided it for JSON documents with node identities: a nested value's cell stays
call-scoped there (`HasUnheldIdentity`), and the document's own cell keeps it across calls.

Pinned by `JsonConformance.ModuleCallsOfAnOwnedDocument` (the expiry rows, skipped for traits without
an `Identity` hook).

**Proposal.** Recognise the library's own `WithOwner` holders in `AccessorIsLent` (a marker base or a
holder type the library owns), and decide "lent again" for them by the parent's adapter, which the
holder keeps; then the rows run for every binding. Check the 0336 model (`FixGenericOwner`) still holds.

**Done when** a fresh argument per call whose nested items a macro writes leaves no cell after the
call, for generic containers and JSON without identities, with the conformance rows unskipped.
