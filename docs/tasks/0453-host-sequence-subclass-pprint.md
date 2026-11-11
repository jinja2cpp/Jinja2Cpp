---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/850
priority: low
area: api
depends: [0441]
touches: [src/host_object.cpp, src/statements.cpp, python/src/adapters.cpp]
shares: [include/jinja2cpp/host_object.h]
---
# pprint of a host list subclass that overrides `__iter__`

**Problem.** Since 0441 a Python `list` or `tuple` subclass is iterated through `iter()`, its
`loop.length` is `len()` of the object, and `+`, `*`, comparisons and `str % x` read its stored
items, all as in Jinja2 3.1.6. Found by the 0441 verifier: `l|pprint` for
`class L(list): __iter__ = lambda self: iter("xy")` and `l = L([1, 2, 3])` prints `[1, 2, 3]`
where Jinja2 prints `['x', 'y']` (`pprint`'s `_safe_repr` iterates a list subclass that keeps
`list.__repr__`). This rendered the same before 0441.

**Proposal.** Check what `pprint.pformat` does for list and tuple subclasses at the pinned
Python, and have `pprint` of a host sequence follow it (through `Iterate` where it iterates).

**Done when.** The example matches Jinja2, with a row next to the 0441 subclass rows in
`python/tests/test_values.py` (built afresh per engine: a deep copy of a list subclass is
rebuilt from its `__iter__`).

**Progress (2026-11-11).** Fixed by 0434: `pprint` of a host sequence reads `Iterate`, so the example
prints `['x', 'y']`; the row is next to the 0441 subclass rows in python/tests/test_values.py.
