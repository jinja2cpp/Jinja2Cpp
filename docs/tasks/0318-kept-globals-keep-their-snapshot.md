---
status: in-progress
priority: high
area: robustness
touches: [src/internal_value.cpp, src/internal_value.h, src/template_impl.h, test/user_callable_test.cpp]
---
# A list or map from the globals keeps the globals it was read from

**Problem.** Found by the 0223 plan (probe 15 in /mnt/project-files/release-2.0/0223-probe/probe.cpp).
A render converts the environment's globals once per thread into a snapshot whose lists and maps
borrow the globals map by reference (`ValuesListAdapter<ByRef>`, `ValuesMapAdapter<ByRef>`). A host
callable that keeps such a list, or a value the template made that holds it, reads freed memory
once the host replaces the global and the next render on the thread drops the old snapshot
(heap-use-after-free under ASan), although the host did nothing wrong.

**Decision.** 0223 D3(a), approved by Ruslan 2026-10-25: a value a callable receives stays valid
after the call, except where it refers to data the host lent (the params). A list or map from
the globals keeps the globals map alive (contract 3.10, values item 2).

**Done when.** The snapshot converts a global with `LendGlobal`: lists and maps borrow through
`ByHeldRef`, which holds the globals map (`shared_ptr`) and passes it to the adapters of their
nested lists and maps; generic containers are copied (which shares the host's accessor); strings
are copied once per snapshot instead of viewed. The per-item render path is unchanged (views of
items, cached nested adapters). Row: `UserCallableTestSingle.KeptGlobalsOutliveTheirSnapshot`.

**Limits.** A string item of a global list is still lent as a view (a `Value` holding a
`string_view` after the conversion for the host), valid while the list that lent it lives, the
generic-list rule of `value.h`. Params stay borrowed (contract 3.10, values item 1). 0049 P2
promotes the same adapters to cells and its P4 restore path must keep `ByHeldRef`'s owner.
