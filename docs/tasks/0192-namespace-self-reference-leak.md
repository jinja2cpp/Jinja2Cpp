---
status: dropped
priority: low
area: robustness
touches: [src/internal_value.cpp#NamespaceAdapter]
---
# A namespace that holds itself leaks

**Problem.** Found by the 0042 verifier under LeakSanitizer:
`{% set ns = namespace(a=1) %}{% set ns.d = ns %}{{ ns }}` leaks 296 bytes in 4
allocations. `NamespaceAdapter` keeps its items in a shared `InternalDict`
(`BySharedMutable`), so storing the namespace in itself is a shared_ptr cycle that outlives
the render. A dict that contains itself (`{% set d = {} %}{% do d.update(x=d) %}`) is
probably the same; a list with `append` too. Python's garbage collector frees such cycles.

The corpus cannot pin it: the case renders correctly, and the sanitizer job runs the
parity suite with `detect_leaks=1`, so it would turn CI red.

**Approach.** Either break cycles when the render (or the template's owner) ends, by
clearing every shared container the render created, or hold self-references weakly.
Measure first: a render-scoped registry of shared containers costs an allocation each.

**Done when.** A unit test that stores a namespace, a dict and a list in themselves runs
clean under the sanitizer configuration.

**Dropped (2026-10-16): duplicate of 0110**, which the fuzzing track filed on 2026-10-04 for the
same cycle (`{% set ns.a = ns %}`). Its findings (dict and list cycles, the LeakSanitizer
figures from the 0042 verifier) are carried into 0110.
