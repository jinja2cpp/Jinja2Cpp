---
status: open
priority: low
area: parity
depends: [0435]
touches: [include/jinja2cpp/template.h, src/template.cpp, python/src/template_module.cpp]
shares: [docs/python.md, python/tests/test_environment_api.py]
---
# Macros of a module keep the filter policies of the load

**Problem.** 0435 made the Python module read `env.policies` at each render and pass them as
`RenderOptions::filterPolicies`. `Template::MakeModule` takes no options, so the macros of
`t.module` and `t.make_module()` keep the policies the template was loaded with; Jinja2 reads
`env.policies` each time a filter runs, so a policy changed after the module was made applies to
its macros there (docs/python.md, Differences).

**Proposal.** A `MakeModule` overload with `RenderOptions` (additive), or a per-call override of the
policies when a module's macro runs from the host; the module passes the current snapshot.

**Done when** a policy changed after `t.module` was made applies to its macros, with a row in
`python/tests/test_environment_api.py` against jinja2.
