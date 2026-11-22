---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/989
priority: low
area: parity
depends: [0212]
touches: [include/jinja2cpp/json_adapter.h, src/internal_value.cpp, src/borrowed_cells.h, src/borrowed_cells.cpp, src/cycle_collector.h, tla/CellProtocol.tla, tla/CellTrace.tla, tla/CellApalache.tla, tla/CellInductive.tla, tla/check.py, tla/trace_check.py, tla/README.md, test/binding/conformance.h]
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

**Decision (Ruslan, 2026-11-06): option b.** A borrowed JSON view the host lends again does not keep
a module's writes across calls: each call starts from the host's data, as the first bullet says. This
is deliberate and follows the 2026-10-26 principle that host data is immutable and calls are
independent. Option a' (re-key the cell under the view's accessor at the call's end, about 100-150
lines and a new model action) stays possible later if a user asks for Python's behaviour. The cost
of b: a host `ValuesList` lent again keeps the writes (0049 P2) while a borrowed JSON view does not.

**Proposal.** The `KeepsDocument()` hook for owned documents, the model's call-scoped freeze, and a
test that pins the borrowed case's per-call behaviour (with a comment naming this decision).

**Done when** the model covers the call-scoped freeze, owned documents keep their cells across calls,
and the borrowed case is pinned as deliberate by a test.

**Result.**
- `IJsonNode::KeepsDocument()` (additive, `false` unless overridden): the library's views return
  whether they hold an owner (`Reflect` of an rvalue, of a copied lvalue or of a `shared_ptr`,
  `ReflectJson` with an owner). A cell of such a view is not call-scoped (`HasUnheldIdentity` in
  internal_value.cpp): its source keeps the document, and so the address, alive, so it lives
  across calls like any owned data and a view lent again keeps the module's writes, as Python's.
  The header now also says that the host does not change a shared document while a Value reads it,
  and that `ReflectJson`'s owner must keep the document alive and unchanged.
- A nested value read through its parent (`d.a`, an aliasing `WithOwner` accessor) keeps a
  call-scoped cell: `DropUnheld` cannot tell such an accessor lent again, so a non-call-scoped cell
  of it stayed for the module's life, one per call with fresh documents (verifier round 1). Writes
  to it still last across calls through the document's own cell, which the write makes and which
  keeps the nested item. Writes to a nested value held under a name of its own (`{% set a = d.a %}`,
  a loop item, `d.its|first`) stay call-scoped, as on master; Python keeps them (verifier round 2,
  task 0572, which also has the older leak without node identities).
- Known cost, verifier round 2: a cycle through a fresh owned document's cell
  (`d.update(self=d)`) now stays until the module goes, one document per call, where the
  call-scoped freeze used to hand it to the collector. Owned `ValuesMap` arguments already behave
  so on master (`IsUnheld` counts the cycle's own link); both are task 0573.
- Tests: `JsonConformance.ModuleCallsOfAnOwnedDocument` (owned, copied and shared documents;
  writes kept across calls, a nested item, `sameas` with the module's value, fresh documents per
  call, the documents freed by the next call's end); it fails on master. `ModuleCallsOfABorrowedDocument` names option (b) at the borrowed case.
- Model: key `J`, `HostChangesJson` and the freeze in `EndModCall` (`FixCallScoped`), invariant
  `CallScopedBounded`; runs `module-json` (pass), `module-json-exhaustive`, and the mutant
  `no-call-scoped-freeze` (`NoForeignCell` in 9 states: the 0212 F1/F2 shape). Exports of a frozen
  view read that cell (`OfFrozen`), as `HostIdentity` makes the cell their identity.
- Cell trace: `cfreeze`/`cfrozen` events and `CallFreeze`/`CallFrozen` in CellTrace.tla; the
  default trace filter adds the JSON module-call tests; trace mutant `call-scoped-stays`.
