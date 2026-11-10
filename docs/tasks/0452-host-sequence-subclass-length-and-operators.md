---
status: open
priority: low
area: api
depends: [0441]
touches: [src/host_object.cpp, src/statements.cpp, python/src/adapters.cpp]
shares: [include/jinja2cpp/host_object.h]
---
# A host sequence whose iteration differs from its items: loop.length and operators

**Problem.** Since 0441 a Python `list` or `tuple` subclass is iterated through `iter()`, as
Python iterates it. Two places still differ from Jinja2 3.1.6 when the subclass overrides
`__iter__` (found by the 0441 verifier):

- `loop.length`, `loop.revindex` and `loop.revindex0` count the items `__iter__` gives. Jinja2's
  `LoopContext.length` is `len(iterable)` when the iterable has one. With
  `class L(list): __iter__ = lambda self: iter("xy")` and `l = L([1, 2, 3])`,
  `{% for i in l %}{{ loop.length }}{% endfor %}` gives `22` against Jinja2's `33` (also for a
  non-sequence object with `__len__` and an `__iter__` of another length).
- Operators that rebuild the sequence through `host::AsSequence` read `Iterate()`, while Python
  uses the real items: `l + ['z']` gives `['x', 'y', 'z']` against `[1, 2, 3, 'z']` (same for
  `l * 2` and a tuple subclass `t + ('z',)`); `'%s-%s' % t` formats `x-y` where Python raises
  `TypeError`; `l|pprint` prints `[1, 2, 3]` where Jinja2 prints `['x', 'y']`.

These ran through `Iterate` before 0441 too; they are not regressions.

**Proposal.** Loop length: when the iterated value is a host object with a `Len`, take
`loop.length` from it, as Jinja2 does. Operators: read a host sequence's items by index
(`Len`/`ItemAtTo`, which the module answers with the base type's storage) where Python uses
the sequence protocol rather than `iter()`; `pprint` should follow Python's `repr`.

**Done when.** The examples above match Jinja2, with rows next to the 0441 subclass rows in
`python/tests/test_values.py` (built afresh per engine: a deep copy of a list subclass is
rebuilt from its `__iter__`).
