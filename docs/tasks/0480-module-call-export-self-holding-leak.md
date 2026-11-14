---
status: in-progress
priority: low
area: robustness
depends: [0424]
touches: [src/internal_value.cpp, src/borrowed_cells.h, src/cycle_collector.h, src/cycle_collector.cpp, src/template_module.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/884
shares: [test/template_module_test.cpp]
---
# A self-holding container a module's call hands a host leaks when the host keeps it

**Problem.** Found while fixing 0424. Since 0424 the host's copy of a list, dict or namespace a
render made walks from its items when the host lets go of it after the render, so a container
that holds itself goes with the host's last reference. A template module's call hands the host
a view of its exports instead (`CallListView`, `CallMapView` with the template's container,
docs/tasks/0332), which 0424 does not cover. With the macro
`{% macro m() %}{% set xs = [1] %}{% set _ = xs.append(xs) %}{% set d = {'a': 1} %}{% set _ = d.update(d=d) %}{% set _ = keep(xs) %}{% set _ = keep(d) %}{% endmacro %}`
and a `keep` callable that stores its argument, two calls and the module's destruction followed by
the host dropping what it kept leak 440 bytes in 7 allocations under LSan (master after 0424). The
call's walk sees the view's copy of the adapter share the storage, so it counts the container as
held from outside; afterwards nothing walks it.

The render's fix does not carry over as it is: after the call, the module's later calls may write
a container the call stored in the module's values, on another thread under the module's lock,
while the host drops its view on its own thread, so a walk from the view would race with them.

**Proposal.** When the view goes, walk from its container under the module's lock (the view would
need a weak reference to the module's state, `TemplateModuleState::mutex`), or hand its items to
the module's collector to walk at its next call or when it goes (`KeptObjects`), which needs no lock
on the host's thread.

**Done when** a `TemplateModuleTest` row with the macro above passes under LSan, and
`Module/keep` does not move in `bench/count.py`.

**Resolution.** The second proposal, without a lock on the host's thread: a module keeps a
`DroppedExports` (cycle_collector.h, a list under its own small mutex), which its registry and
each call's `CallExports` see weakly. When the host lets go of a `CallListView`/`CallMapView` of a
template container, or of the last view of a call (`~CallExports`, for the storages it pinned of the
module's cells), the container goes now if the view held it and its items alone and none of them
is a container; else it is handed to the module, whose next call notes its items to that call's
walk (`NoteDroppedExports`) and whose destructor walks what is left (`CollectDroppedExports`)
after closing the list; once the module has gone, the host's thread walks it as a render's host
copy does (0424). Rows: `TemplateModuleTest.KeptSelfHoldingExportGoesWithTheHostsValue`, six
sources (list, dict, namespace, list-dict cycle, stored in the module, the module's cell of a
host list) times three orders (module first, host first, host then a later call), LSan clean.

Limit: a container the host lets go of while the module lives, and that held other containers, is
freed at the module's next call or when it goes, not at once (a host that drops many such
containers between calls holds them until then). A view of the module's cell needs nothing: the
module's own walk reaches the cell while it lives, and the last view's `~CallExports` walks the
pinned storages once it has gone. `Module/keep`: +0.98% instructions (+70, mostly the weak reference each call's exports
take to the module and drop) and +16 bytes per call (that reference in `CallExports`).
