---
status: open
priority: medium
area: evaluator
depends: [0003]
touches: [src/internal_value.cpp, src/statements.cpp#AssignTo]
---
# A namespace that holds itself leaks

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
must not recurse forever either: check it against Python's `[...]` output.

**Done when.** The input above is in `fuzz/regressions/` and replays clean under
LeakSanitizer, and a parity case covers the self-referencing namespace.
