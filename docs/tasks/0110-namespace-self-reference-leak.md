---
status: done
priority: high
area: evaluator
depends: [0003]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, src/internal_value.cpp, src/statements.cpp#AssignTo, src/value_methods.cpp, src/template_impl.h]
---
# A namespace that holds itself leaks

**Release impact (0221).** Ordinary template evaluation can retain allocations
after the render ends. Treat this as an embedding/lifetime correctness candidate
for the release gate, not merely a difference in Python garbage collection. The
zero-crash parity summary does not cover this known leak outside the corpus.

**Problem.** Found by the `fuzz render` job on PR #376 (2026-10-04, unrelated to that PR's
include changes). The minimized input is:

```
{% set ns = namespace(a=1) %}{% set ns.a = ns %}
```

It stores the namespace in one of its own attributes. Namespaces are `BySharedMutable` map
adapters held by `shared_ptr`, so the map now owns a reference to itself. Nothing frees it,
and LeakSanitizer reports the `OrderedMap` node, the adapter and the control block
(264 bytes in 3 allocations). In Python the garbage collector breaks the cycle. The same
applies to a dict or list that is mutated to contain itself (`d.update({'self': d})`,
`l.append(l)`) wherever such writes are supported.

**Ideas.** Python allows the cycle and renders `ns.a.a.a` fine, so refusing the
assignment would be a divergence. One way out is for the render context to own
every mutable container created during a render, with values holding them weakly or by
raw pointer, and to free them all when the render ends. A cheaper way is to clear the
maps of the namespaces a render created (tracked in the render context) when the
render ends, which breaks any cycle among them. Rendering such a value (`{{ ns }}`)
must not recurse forever either; Python prints `<Namespace {'a': <Namespace {...}>}>`.

**Done when.** The input above is in `fuzz/regressions/` and replays clean under
LeakSanitizer, and a parity case covers the self-referencing namespace.

**Also found by the 0042 verifier (filed as 0192, folded in here 2026-10-16).**
`{% set ns = namespace(a=1) %}{% set ns.d = ns %}{{ ns }}` leaks 296 bytes in 4 allocations
on master after 0042, which made namespaces objects. A dict that contains itself
(`{% set d = {} %}{% do d.update(x=d) %}`) and a list that appends itself are probably the
same cycle. The sanitizer job runs the parity suite with `detect_leaks=1`, so a corpus case
would turn CI red until this is fixed; pin it with the fix. A unit test that stores a
namespace, a dict and a list in themselves, run under the sanitizer configuration, is part
of Done when.

**Resolution (PR link added on merge).** Python's behaviour, with the cycles freed when the
render ends. `CycleCollector` (src/cycle_collector.h) lives for each render, made before
anything the render keeps values in. A store of a container in a list, dict or namespace
(`set ns.attr`, `append`, `extend`, `insert`, `update`, `setdefault`) notes the receiver.
When the render ends, the collector walks the containers reachable from those it noted,
counts the references each gets from the others, and treats one that has more as held from
outside (a user callable, a cache), with everything it reaches; it empties the rest, which
frees their cycles, as Python's collector does. It follows only containers whose accessor
alone holds their items (`OwnsMutableItemsAlone`), so it may miss a cycle but never empties
a container anything else still sees. A render inside another on the same thread hands its
notes to the outer one. Self-containing lists and dicts, which 0020 refused, are allowed
again; `append()`, `update()` and the like still refuse a cycle through a bound method or a
key-value pair (0049), and cycles through other objects (the `loop` object, `cycler()`, `groupby` tuples) still leak
(0251). Cases `statements.namespace_contains_itself*`, `methods.*_contains_itself*`;
regressions `fuzz/regressions/namespace-contains-itself.j2` and
`list-and-dict-contain-themselves.j2`; unit tests `ExpressionTest.SelfContainingContainers`
and `CycleFreedKeepsListHeldByCaller`.
