---
status: done
priority: medium
area: robustness
depends: [0003]
touches: [src/internal_value.h, src/internal_value.cpp, src/generic_adapters.h]
shares: [src/value_visitors.h, src/serialize_filters.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/529
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

**Done.** Destruction: `ValueObject` keeps its own reference count instead of
`boost::intrusive_ref_counter`, and its last release calls `detail::DestroyValueObject`,
which deletes up to 64 nested objects directly and puts deeper ones on a per-thread list
that the outermost deletion empties in a loop. The list is a plain struct with a `malloc`ed
buffer, freed when it empties, so it needs no thread_local destructor. 100000 nested lists
or dicts render `ok` (`fuzz/regressions/deep-nested-value.j2`, `RecursionLimitsTest.DeepValues`).

Walks: printing (`ValueRendererBase`, which also no longer cuts nesting off at 256 levels
with `[...]`), `pprint`, `tojson` (which failed with `InvalidValueType` past 1000 levels;
Python prints 5000) and list and dict comparison call `CheckStack()` per level and end in
`RecursionLimitExceeded`. Comparison first checks identity, as Python's does for items: a
list or dict equals itself without its items being compared (`[nan] == [nan]` through one
variable is `True`), which also keeps `ns.v == ns.v + []` shallow. `length` and lazy filter
chains (`ns.v = ns.v|select`, `|reverse`, `|map`, slices) needed nothing: 100000 levels of
each render. Corpus: `operators.eq_same_list_nan`, `operators.eq_same_dict_nan`,
`output.nested_list_repr_deep`, `output.nested_dict_repr_deep`.

Left as is: Python also treats one float object as equal to itself, so `[n] == [n]` with
`n` NaN is `True` there; Jinja2C++ stores numbers inline, without an identity, and prints
`False`. Not worth boxing numbers for.

Follow-up: train 13's counts put Render/chat_* up to +1.34% and strings at +1.05%, on the
cases that render through `tojson` or the value printer. That cost is task 0204.
