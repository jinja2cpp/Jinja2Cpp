---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/672
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
are copied once per snapshot instead of viewed. String items and keys of a held list or map
are copied when read (a view kept alone, as in `keep(g[0])` or `keep([k, v])` of a dictsort,
would not hold the map); integers and nested containers are as before. Rows:
`UserCallableTestSingle.KeptGlobalsOutliveTheirSnapshot` and
`UserCallableTestSingle.KeptGlobalItemsOutliveTheirSnapshot`.

**Limits.** Params stay borrowed (contract 3.10, values item 1). 0049 P2
promotes the same adapters to cells and its P4 restore path must keep `ByHeldRef`'s owner.
