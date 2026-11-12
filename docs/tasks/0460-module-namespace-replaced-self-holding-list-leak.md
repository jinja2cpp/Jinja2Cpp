---
status: open
priority: medium
area: robustness
depends: []
touches: [src/borrowed_cells.cpp, src/cycle_collector.cpp, src/template_module.cpp, test/template_module_test.cpp]
---
# A self-holding list a module call stores in a namespace leaks once a later call replaces it

**Problem.** Found by the verifier on the PR for 0347/0407: a module
`{% set ns = namespace(v=none) %}{% macro n(d) %}{% set c = [] %}{% set _ = c.append(c) %}{% set ns.v = c %}{% endmacro %}`
called twice leaks 112 bytes under LSan, on master c9fe6ed too. The first call's `c` holds
itself and is held by `ns.v`, so the first call's collector counts it as held from outside
(the module's names, ModuleScope::LiveRoots). The second call's `{% set ns.v = c %}` drops the
first `c`; nothing notes it then, its note went with the first call's collector, and the
module's destructor walks from the names, which no longer reach it.

**Proposal.** When a store into a module's namespace (or other live root) replaces a container
value, note the old value to the running call's collector, so the walk at the call's end finds
the cycle unheld. Check the same for a dict item a call replaces (`d.update`, `d[k] = v`).

**Done when** a `TemplateModuleTest` row with two calls of `n` (and the dict variant) passes
under LSan, and `Module/keep_written` does not move in `bench/count.py`.
