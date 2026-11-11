---
status: in-progress
priority: medium
area: robustness
depends: []
touches: [src/borrowed_cells.cpp, src/cycle_collector.cpp, src/template_module.cpp, test/template_module_test.cpp]
---
# Leaks in template module calls through call exports and closure frames

**Problem.** The 0212 P3 verifier (#778) found two leaks that LSan reports with the same byte
counts on master (76359054) and on #778's head, so they predate it:
- A render passes its own dict through a host callable into a module macro that writes it:
  module `{% macro n(d) %}{% set _ = d.update(l=[1]) %}{% endmacro %}`, render
  `{% set d = {'a': [1]} %}{{ cbn(d) }}` where the host callable `cbn` calls `n`. 1328 bytes leak,
  all indirect, through `CallExportsWith<CallListView>` (internal_value.cpp, ExportCallList) and a
  cell adopted in `CellLink::Adopt`. The template makes no cycle. The render's `d|list` then shows
  `['a']` where Python gives `['a', 'l']`; check whether that is the recorded copy-on-write
  divergence or a separate one.
- A module list that holds itself (`{% set _ = cap.append(cap) %}`), captured by an inner macro the
  module stores, leaks 208 bytes when the module goes, through the closure's kept frame (0373).

**Proposal.** Reproduce both as `TemplateModuleTest` rows under LSan; for the first, find what keeps
the export and the cell alive past the render (a reference from the cell's storage back to its
CallExports?); for the second, make the module's collector reach the cycle through the closure's
frame (ClosureCapture traversal or the module's KeptObjects).

**Done when** both rows pass under LSan and the `d|list` result is either Python's or on the
divergence allow-list with its reason.

**Resolution (2026-11-11).** Both rows are `TemplateModuleTest` cases, LSan-clean.
- `RenderDictWrittenThroughAHostCallIntoAModule` (1552 bytes on master c9fe6ed): the module's
  `d.update` copied the render's dict through the host's lent view, and the render's `[1]`
  inside it was exported as the module call's own data (a `CallListView` of the call's exports,
  because a module call was running on the thread). The module's copy of `d` then held the
  call's exports, which pin that copy: a cycle no collector sees. A lent view now reads in the
  render that lent it (`LendFrameScope` makes the lease's collector current while the lease is
  active, none after), so the render's list is a view of the render's data, never the call's.
  `d|list` stays `['a']`: the host holds a read-only snapshot of the render's dict, so the
  macro writes a copy of its own (docs/api-2.0.md 3.10, item 9), a deliberate divergence from
  Python, which shares one dict. It cannot be a parity corpus case: it needs a host callable
  that calls a module macro.
- `SelfHoldingListCapturedByAStoredClosure` (112 bytes): the module's destructor released the
  closures' kept copies of names before its walk, and the cycle in them had been noted only to
  the call's collector. `KeptObjects` now knows which kept object is a copy of names
  (`KeepUntilEnd`'s third argument) and the destructor notes their values first.
