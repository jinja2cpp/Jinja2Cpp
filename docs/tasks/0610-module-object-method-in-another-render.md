---
status: open
priority: low
area: parity
depends: [0594]
touches: [src/template_module.cpp#ModuleObjectView, src/internal_value.cpp#GenericMapAdapter]
shares: [python/tests/test_module.py, docs/python.md]
---
# A module object's method read in another render prints empty, and a cycler's items is a list

**Problem.** Since 0594 a module's namespace, cycler or joiner passed to another render is that object there. Two reads
still differ from Jinja2 3.1.6, with `m = Template("{% set c = cycler(1, 2) %}{% set j = joiner('-') %}").make_module()`:

| template, `c=m.c`, `k=m.j` | Jinja2 | Jinja2C++ |
|---|---|---|
| `{{ c.reset }}` | `<bound method Cycler.reset of <jinja2.utils.Cycler object at 0x...>>` | empty |
| `{{ c.next }}`, `{{ k.__call__ }}` | the bound method | empty |
| `{{ c.items }}` | `(1, 2)` | `[1, 2]` |

The module's view (`ModuleObjectView`) copies each attribute for the host: a method becomes a host callable, which
prints empty in the other render, and the cycler's tuple of items becomes a list. In the module's own render both print as
Jinja2's.

**Proposal.** Let the copy of a method of a live object keep its bound-method name (as `SetBoundMethod` does for
`loop.__call__`), and the cycler's `items` a tuple, in Objects mode.

**Done when.** A `python/tests/test_module.py` row renders the three templates above as Jinja2 does (addresses
normalised), and the docs/python.md sentence naming 0610 is gone.
