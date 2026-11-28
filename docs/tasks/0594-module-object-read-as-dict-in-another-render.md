---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1031
priority: low
area: parity
depends: [0575]
touches: [src/internal_value.cpp#GenericMapAdapter, src/value_visitors.h#ValueRendererBase, src/serialize_filters.cpp]
shares: [python/tests/test_module.py, docs/python.md]
---
# A module's namespace, cycler or joiner passed to another render is read there as a dict

**Problem.** `make_module` exports a namespace, a cycler and a joiner as live objects (0411), and 0575 made one passed
to a render print as Jinja2's object. Everything else in that render reads it as a dict, as the engine reads any host
map (`GenericMapAdapter` reports no object class): with `m = Template(src).make_module()` and
`src = "{% set ns = namespace(count=0) %}{% set j = joiner('-') %}"`, Jinja2 3.1.6 vs Jinja2C++:

| template, `x=m.ns`, `k=m.j` | Jinja2 | Jinja2C++ |
|---|---|---|
| `{{ x\|pprint }}` | `<Namespace {'count': 0}>` | `{'count': 0}` |
| `{{ x is mapping }}` | `False` | `True` |
| `{{ x\|length }}` | `TypeError` | `1` |
| `{% for a in x %}{{ a }}{% endfor %}` | `TypeError` | `count` |
| `{{ x.items()\|list }}` | `UndefinedError` | `[('count', 0)]` |
| `{{ k() }}{{ k() }}` | `-` | `'dict' object is not callable` |

The same holds for a namespace a template lent to a C++ host and the host passes to another render.

A namespace that holds itself (`{% set ns.me = ns %}`) passed to another render fails there with
`RuntimeError: bad_expected_access` for `{{ x }}`, `{{ [x] }}`, `|string`, `|pprint`, `|e` and `x.me == x`, where Jinja2
prints `<Namespace {'n': 1, 'me': <Namespace {...}>}>` (each read of the view is a new host map, so the printer's
container stack never sees the repeat, and the nesting runs into the depth limit); a 5000-deep chain of module
namespaces does the same for `|tojson`. The error is not mapped to a template error.

**Proposal.** Let `GenericMapAdapter` report the host map's object class for the live classes (`Namespace`, `Cycler`,
`Joiner`) without making it a namespace the render may `set` on (`IsNamespace` stays false, statements.cpp), check
every reader of `GetObjectClass` (iteration, `is mapping`, length, key identity, the sandbox's synthetic attributes)
and `|pprint`, and let a call of a Joiner-class host map call its `__call__`.

**Done when.** A `python/tests/test_module.py` row renders each template above, and the self-holding namespace, as Jinja2 does, and the docs/python.md
sentence naming 0594 is gone.
