---
status: in-progress
priority: low
area: robustness
depends: [0459]
touches: [src/internal_value.h, src/internal_value.cpp]
shares: [docs/security.md]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/863
---
# Count the objects the render memory budget leaves out

**Problem.** `Settings::Limits::maxRenderMemory` (0459) counts the engine's strings
(`StringObject`), list slots (`BySharedMutable` storage) and dict entries (`TemplateDict`).
Other objects a template can keep are counted only by the 32-byte slot that holds them: loop
objects, macros and other callables, bound methods, undefined values' info, generator state,
big integers (`BigIntObject`) and the anchors of reflected items. A template that appends
`loop` (or `caller`, or a big integer) to a list on every step keeps a few hundred bytes per
step while the budget sees 32, so under `UntrustedLimits()` (10^7 steps) such a list can reach
about a gigabyte before the step limit stops it.

**Proposal.** Charge these objects where they are made (`MakeRef` of the classes above, or a
fixed charge per `ValueObject` the render makes) and give it back in their destructors, as
`StringObject` does. Measure first: the charge runs once per macro call and loop, so
`bench/count.py` decides whether a fixed per-object estimate is cheap enough.

**Tests.** Rows in `test/limits_test.cpp` `Cases()` with `maxRenderMemory = 1 << 20`:
`{% set l = [] %}{% for i in range(100000) %}{% set _ = l.append(loop) %}{% endfor %}` and the
same with a big integer (`2 ** 1000`), each failing with `maxRenderMemory`.

**Trust model.** When this lands, drop its entry under Known gaps in [docs/security.md](../security.md).

**Resolution.** `ValueObject` has class-specific `operator new`/`operator delete` that charge and give back the
object's own size (the size the deleting destructor passes), which covers every engine object at once: loop state,
callables, bound methods, undefined info, generators, anchors. An allocation function may throw only
`std::bad_alloc`, so past the budget `ChargeObjectMemory` notes an overdraft and makes the next step take the slow
path, which fails the render with `maxRenderMemory`. `BigIntObject` charges its digits in `Make`; `StringObject`
keeps its own count. Rows in `Cases()`: kept nested-loop objects, macros, undefined values and big integers fail,
the dropped and plain-int controls pass; each kept row passes without the change.

What outlives the render is made under `UncountedMemoryScope`: a loaded template (`TemplateEnv::LoadTemplate`),
the thread's snapshot of the globals and the built-in globals, which the first render alone paid for otherwise
(`LimitsCachesTest.FirstRenderPaysForNoCache`).

A reload `EveryUse` drops as unchanged (0313) is freed uncounted too, or freeing it would credit the render with
bytes it never paid (security review; `LimitsCachesTest.DroppedReloadGivesNoBudget`). The general rule, release only
what was charged, is [0483](0483-render-memory-uncharged-release.md).

**Next.** Objects made with `std::make_shared` (accessor copies, closure captures) bypass the allocation function
and stay uncounted. What an object allocates apart (a loop's vectors, an undefined value's name strings) is still not
counted; the per-object size is a lower bound.
