---
status: open
priority: low
area: robustness
depends: [0459]
touches: [src/internal_value.h, src/internal_value.cpp]
shares: [docs/security.md]
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
