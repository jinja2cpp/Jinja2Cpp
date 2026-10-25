---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/663
priority: low
area: evaluator
depends: [0110]
touches: [src/cycle_collector.cpp, src/internal_value.cpp#NamedTupleAccessor, src/global_functions.cpp#CallCycler, src/statements.cpp#LoopFrame]
---
# Reference cycles through objects the cycle collector cannot follow leak

**Problem.** 0110 frees cycles among lists, dicts and namespaces when a render ends
(`CycleCollector`), but it follows only containers whose items it can enumerate and empty.
A cycle that runs through any other object keeps itself alive after the render, and
LeakSanitizer reports it (the first three seen with the sanitizer fuzz build, 2026-10-18):

```
{% set ns = namespace() %}{% for i in [ns] %}{% set ns.x = loop %}{% endfor %}
{% set ns = namespace() %}{% set ns.c = cycler(ns) %}
{% set ns = namespace() %}{% set ns.l = [ns]|groupby('x') %}
{% set ns = namespace() %}{% set ns.l = [ns] %}{% set ns.f = ns.l.append %}
```

The `loop` object keeps the iterated list, `cycler()` its items (an `ObjectMapAdapter` held
`ByVal`) and a `groupby` group is a namedtuple (`NamedTupleAccessor`) around its list. A
value passed through a user callable and back (a `GenericMap` copy of the accessor) is
another object the walk cannot see into, and so is a bound method stored in a namespace
(`{% set ns.l = [ns] %}{% set ns.f = ns.l.append %}`): unlike `append()` and `update()`,
`set ns.attr` does not refuse a cycle through one, since the walk that finds it would make
`set ns.v = [ns.v]` in a loop quadratic. Python frees all of these.

The refusal at the store (`NoteContainerStore`, 0049) has costs of its own, found by the
0110 verifier: it refuses a cycle through a key-value pair that Python accepts
(`{% set d = dict(a=l) %}{% for p in d|dictsort %}{% do l.append(p) %}{% endfor %}`), it
walks everything the stored value reaches on every store, so a tree with parent links
(`d.update(r=root)` then `root.append(d)` in a loop) is quadratic (N = 20000 takes about a
minute in Release), and the walk recurses once per level, so a 200000-deep chain closed by
one `append` overflows the stack (as on master before 0110). Following these objects in the
collector would let the walk go.

**Ideas.** Give the walk what CPython's `tp_traverse`/`tp_clear` give its collector: a
virtual on the accessors that holds values (namedtuple, `ObjectMapAdapter`, the loop frame)
that visits the values it holds and one that drops them, with the same rule as
`OwnsMutableItemsAlone` (drop only what nothing else shares). The namedtuple is the
cheapest: it is a list adapter around another list. A cycle through a bound method or a
key-value pair is refused at the store instead (0049); the same traversal would let those
be allowed too.

**Done when.** The inputs above are in `fuzz/regressions/` and replay clean under
LeakSanitizer, and `methods.list_contains_its_bound_method` no longer needs its 0049 line.
