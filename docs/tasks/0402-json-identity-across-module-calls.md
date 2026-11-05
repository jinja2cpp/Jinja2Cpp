---
status: open
priority: low
area: parity
depends: [0212]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, src/cycle_collector.h, tla/CellProtocol.tla, test/binding/conformance.h]
---
# JSON values across the calls of a template module

**Problem.** 0212 P3 keys the cell of a JSON value by the value's address
(`IJsonNode::NodeIdentity`), so two reads of one value are one object within a render or a module
call. A borrowed document (`jinja2::Reflect(const J&)`) is not kept alive by the cell, and the host
may free or reassign it between two calls of a template module; a cell found at that address in a
later call would show the earlier call's document (verifier finding on the 0212 P3 PR). P3 therefore
drops such a cell between two calls once the host can no longer lend its source again
(`BorrowedCell::m_unheldIdentity`, `ModuleCallCollector::DropBetweenCalls`; test
`JsonConformance.ModuleCallsOfABorrowedDocument`). What remains differs from Python, as on master
before P3:
- Once the host has let go of every view, a fresh view of the same, unchanged document in each call
  starts clean: Python passes the same dict, so `{% macro g(l) %}{{ l }}{% set _ = l.append(1) %}{% endmacro %}`
  called twice with a fresh `Reflect(doc)` prints `[0]` both times where Python prints `[0]`, `[0, 1]`.
  While the host keeps a view and lends it, or another view of the same value, the writes are kept.
- A nested value of a kept view (`d.l` of a kept `d`) is read through a fresh accessor in each call,
  so its cell goes between the calls too.
- The TLA+ model (tla/CellProtocol.tla, `Unheld`) has no identity the source does not keep alive:
  the rule "drop between calls, never during one" is not checked by TLC or the trace validation.

**Proposal.** Model unheld identities in CellProtocol.tla (an address the host may reuse once it lets
go of its views) and check that no call reads a cell of another value. Then decide whether a cell may
outlive the calls when the host promises the document's lifetime (for example an owned or shared
view, whose cell keeps the document alive), which would give Python's behaviour for the first two
cases.

**Done when** the model covers unheld identities, and the two cases above either match Python or
are recorded as deliberate divergences with a test.
