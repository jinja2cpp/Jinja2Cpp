---
status: in-progress
priority: low
area: parity
depends: [0411]
touches: [src/template_module.cpp, python/src/template_module.cpp]
shares: [python/tests/test_module.py, docs/python.md]
---
# `make_module(shared=True)` keeps the engine's own globals

**Problem.** Jinja2's `Template.make_module(vars, shared=True)` renders with `vars` only: no environment
globals and no built-ins, so `range`, `cycler`, `namespace`, `dict` and `lipsum` are undefined there
(checked with Jinja2 3.1.6). The Python module leaves out `env.globals`, but `TemplateModuleState::Render`
always gives the module the engine's built-in table (`GetBuiltinGlobals`, src/template_module.cpp), so
`{{ range(2)|list }}` renders under `shared=True` where Jinja2 raises an UndefinedError. Split from 0411.

**Proposal.** A `ModuleOptions` field (0411 added the struct, include/jinja2cpp/template.h) such as
`bool builtinGlobals = true` that leaves the table out of the module's context; decide what an include
or import inside the module sees (Jinja2: the shared context's vars only). An ABI change of
`ModuleOptions`: before G1 if possible.

**Done when** a pytest row compares `make_module({'x': 1}, shared=True)` of a template calling `range` and
`namespace` with Jinja2, and the docs/python.md sentence on `shared=True` goes.
