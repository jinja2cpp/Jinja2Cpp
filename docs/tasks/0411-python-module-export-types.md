---
status: open
priority: low
area: parity
depends: [0354, 0284]
touches: [python/src/template_module.cpp, src/template_module.cpp]
shares: [python/tests/test_module.py, docs/python.md]
---
# `make_module` exports lose the type of a namespace, a cycler, a range and an undefined

**Problem.** `Template::MakeModule` hands the host a `ValuesMap` of public `Value`s, which have
no namespace, cycler, range or undefined. The Python module therefore gives a `dict` for
`namespace(...)` and `cycler(...)` (Jinja2: `jinja2.utils.Namespace`, `Cycler`), a `list` for a `range`,
`None` for an exported `Undefined`, and spells a non-str dict key with `str()`. A Python object
passed as a param goes through the host adapter and comes back as a map that cannot list its keys
(an object with attributes exports as `{}`). `shared=True` cannot drop the engine's own globals.
A list or dict that a template reads off a Python object (`{% set t = users[0].tags %}`) exports as `{}`:
the export copy cannot enumerate a host object. `sorted(...)`/`dictsort` entries (tuples in Jinja2) export as `{key, value}` maps.
None of this is a crash; code that uses `module.ns.count` or `module.cyc.next()` fails.

**Proposal.** An internal export path for the Python module (not the public `MakeModule`):
`TemplateImpl<char>::MakeModuleInternal` returning the exports as `InternalValue`s with the
module's lifetime anchors (the same `TemplateModuleState`), so that `ToPython` converts them as
it converts a render's values: a namespace as a live object bound to the module (a
`TemplateValue` with a module lease instead of a call lease), a Python object as itself.
Decide with the 0284 owner whether that is one more option of the public `MakeModule` instead.

**Done when** pytest cases compare `module.ns.count`, `module.cyc.next()`, an exported Python
object's identity and an exported `Undefined` with Jinja2's, and the docs/python.md paragraph on
exports is shortened accordingly.
