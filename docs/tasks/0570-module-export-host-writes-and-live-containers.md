---
status: open
priority: low
area: parity
depends: [0411]
touches: [src/template_module.cpp, python/src/template_module.cpp]
shares: [python/tests/test_module.py, docs/python.md]
---
# Writes to a module's exports from Python, and live exported lists and dicts

**Problem.** After 0411 an exported `namespace` or `cycler` of `make_module` and `Template.module` is a live
view of the module's object, but what is left differs from Jinja2:
- `m.ns.count = 10` from Python: Jinja2 sets the attribute and the macros see it; the view refuses it.
- An exported list or dict is a copy: a macro's `{% do l.append(1) %}` (or `l.append` through a host call)
  does not show in `m.l`, and Python's `m.l.append(2)` does not reach the macros. Jinja2 shares the object.
- `isinstance(m.ns, jinja2.utils.Namespace)` is false, each read of a nested namespace is a new Python object
  (`m.ns.inner is m.ns.inner` false), and `m.cyc.next` prints as a module macro.

**Proposal.** A write path on the view (under the module's lock, the value converted as a call argument is).
Live lists and dicts conflict with the design principle (no shared mutable objects across renders; a module
is one context, so the same exception as live namespaces may apply): a decision for Ruslan before any code.

**Done when** pytest rows compare the cases above with Jinja2, or the ones decided against are listed in
docs/python.md as deliberate divergences.

**Note.** A `ModuleObject` passed to a render prints as its copy (`{'count': 0}`); Jinja2 prints a namespace as
`<Namespace {'count': 0}>`. Decide it with the write path.
