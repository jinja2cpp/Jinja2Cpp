---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/952
priority: low
area: parity
depends: [0500]
touches: [src/value_methods.cpp#StrOps, python/src/]
shares: [python/tests/test_values.py]
---
# str.format ignores a host object's __format__ outside the sandbox

**Problem.** Found by 0500. Python's `str.format` formats each field with `format(value, spec)`, which calls the
object's `__format__`. The Jinja2C++ sandbox's formatter does (the module's `Format` policy), but the plain
`str.format` and `format_map` of the engine never ask a host object: through the Python module, with
`class F: __format__ = lambda s, spec: 'F<' + spec + '>'; __str__ = lambda s: 'S'`,

| Template | Jinja2 3.1.6 | Jinja2C++ |
|---|---|---|
| `{{ '{0}'.format(o) }}` | `F<>` | `S` |
| `{{ '{0:>4}'.format(o) }}` | `F<>4>` | TemplateRuntimeError `unsupported format string passed to F.__format__` |

So a `__format__` that raises StopIteration (0500's undefined) or anything else is not reached either.

**Proposal.** Let a host object format itself (a `HostObject` hook, or the module's format policy used outside the
sandbox too) where the field's value is a host object, with the spec; keep the engine's own formatting for its
own values.

**Done when.** Both templates match Jinja2 through the module, with rows in python/tests/test_values.py, and
`'{0}'.format(o)` with a raising `__format__` gives 0500's undefined.
