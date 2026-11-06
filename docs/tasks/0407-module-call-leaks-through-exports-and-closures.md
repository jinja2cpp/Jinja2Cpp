---
status: open
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
