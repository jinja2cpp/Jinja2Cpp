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
may change or free it between two calls of a template module, while views of it are alive or not; a
cell found at that address in a later call would show the earlier call's document (verifier findings
F1 and F2 on #778). P3 therefore makes these cells call-scoped: at the end of a module's outermost
call they freeze and leave the registry (`BorrowedCells::FreezeCallScoped`, the advisor's option g;
test `JsonConformance.ModuleCallsOfABorrowedDocument`). What differs from Python:
- A view the host keeps and lends to every call starts from the host's data in each call: Python
  passes the same dict, so `{% macro g(l) %}{{ l }}{% set _ = l.append(1) %}{% endmacro %}` called
  twice with one kept `Reflect(doc)` prints `[0]` both times where Python prints `[0]`, `[0, 1]`.
  Before P3 that view kept the writes (0049 P2, the accessor key); a fresh view never did.
- A view the module keeps (`{% set _ = store.append(d) %}`) keeps what the call left in it and
  copies on a later write, as a value kept after a render does (0331).
- Owned or shared documents (`Reflect(J&&)`, `Reflect(shared_ptr)`) are kept alive by the cell and
  cannot change, so their cells could live across calls with Python's behaviour; the library cannot
  tell them from borrowed ones without a hook (`IJsonNode::KeepsDocument()`, additive, before G1).
- The TLA+ model (tla/CellProtocol.tla) and the cell trace have no call-scoped freeze: it is the
  render's FinishFreeze for a subset of the cells at a module call's end, not checked by TLC.

**Decision for Ruslan.** Should a borrowed JSON view the host lends again keep a module's writes
across calls (0049 P2 for JSON)? If yes: re-key the cell under the view's accessor at the call's end
and let `CellLink::Find` try that key on a miss (advisor option a', about 100-150 lines and a new
model action). If no: record the first bullet as deliberate.

**Proposal.** The `KeepsDocument()` hook for owned documents, the model's call-scoped freeze, and the
decision above.

**Done when** the model covers the call-scoped freeze, owned documents keep their cells across calls,
and the borrowed case either matches Python or is recorded as deliberate with a test.
