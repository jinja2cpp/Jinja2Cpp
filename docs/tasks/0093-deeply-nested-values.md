---
status: open
priority: medium
area: robustness
depends: [0003]
touches: [src/internal_value.h, src/internal_value.cpp, src/generic_adapters.h]
---
# Deeply nested values overflow the stack when they are destroyed or printed

**Problem.** Found by the fuzzing track (0003). A loop can nest a value as deep as it
iterates, and every recursive walk over the value then recurses once per level. The
template is 100 bytes; Debug GCC, 8 MiB stack:

```
{% set ns = namespace(v=[]) %}{% for i in range(20000) %}{% set ns.v = [ns.v] %}{% endfor %}ok
```

renders `ok` at 5000 levels and segfaults at 20000 while the last reference to the outer
list is released: each `InternalValue` destroys its list, which destroys the next
`InternalValue`, and so on (the backtrace is a chain of `std::variant` destructors).
Printing (`{{ ns.v }}`), `tojson`, `length` of such a value and `==` between two of them
recurse the same way. The depth limits of 0003 do not apply: they count template nesting,
not value nesting, and a destructor cannot stop with an error. Python Jinja2 renders the
template; CPython destroys nested containers through its "trashcan", which defers deep
deallocations to a loop, and raises RecursionError when printing them.

**Proposal.**
- Destruction: give the list and map storage a trashcan of its own: a destructor that
  moves the children whose reference count drops to zero onto a thread-local pending list
  and drains it in a loop at the outermost level.
- Walks over values (printing, serializers, comparison, `length` of nested adapters):
  call `CheckStack()` from `src/recursion_guard.h` at each level, so that they end in
  `ErrorCode::RecursionLimitExceeded` like Python's RecursionError.

**Done when.** The template above with 100000 levels renders `ok` under ASan, printing
the value fails with `RecursionLimitExceeded`, and the input sits in `fuzz/regressions/`.
