---
status: in-progress
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/870
area: robustness
depends: [0357]
touches: [src/cycle_collector.cpp]
shares: [test/public_host_object_test.cpp]
---
# A template container that holds itself leaks when a host keeps its export

**Problem.** Found by the 0357 verifier (LSan). A C++ callable that stores the `Value` it gets for
`{% set xs = [] %}{% do xs.append(xs) %}{{ keep(xs) }}` (or a dict that holds itself) keeps the
storage alive past the render, and when the host drops it the storage is never freed: the cycle
collector skips a container while an export holds a reference to it, and after the render nothing
collects it. Probably as old as the export views (0331/0348); not checked on master.

**Proposal.** Check master first. Then either let the host's last reference break the template's
own cycle (the export view clears the container it keeps alone, as `Clear` does for the
collector), or document that a host that keeps a self-holding container owns the cycle. Row in
`test/public_host_object_test.cpp` under the ASan job.

**Resolution.** Checked on master: both rows leaked. The host's copy of a list, dict or namespace a
render made (`CreateGenericList`, `CreateGenericMap`, the snapshot view of a dict with keys that
are not strings) now knows whether its render has ended (`CycleCollector::ExportToken`, which
expires when the outermost render's walk starts). When the host lets go of it after that, it
walks from the container's items (`CycleCollector::CollectExport`), which frees a container that
holds itself and is held by nothing but its own items now, as Python's collector does. Before the
render has ended nothing changes: its own walk sees the copy gone. The walks of host copies run
one at a time and after the walk of the render that made them, under one mutex that renders
which handed the host a container take. A copy dropped on another thread while its render ends
may miss the walk and leak, never race. A walk can free a host's copy itself (the host handed a
kept container back to a template): that copy's walk is queued and runs after the walk on the same
thread rather than waiting for the lock it holds
(`PublicHostObjectTest.KeptContainerHandedBackGoesInsideTheRendersWalk`). Module calls' exports
(`CallListView`) are not covered: task 0480.
