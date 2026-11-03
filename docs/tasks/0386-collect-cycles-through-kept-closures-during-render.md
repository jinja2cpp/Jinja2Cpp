---
status: open
priority: low
area: perf
depends: [0373, 0375]
touches: [src/cycle_collector.cpp, src/cycle_collector.h, src/render_context.h, src/statements.cpp]
---
# Free cycles through a kept closure's copy while the render runs

**Problem.** Found while fixing 0373 (item 4). A closure stored in a list, dict or namespace of
its own frame, which reads that container, closes a cycle through the copy of the frame the
closure keeps (copy, container, closure, capture): the render keeps the copy while the capture
lives (`CycleCollector::KeepUntilEnd`), and the capture lives while the closure in the
container does. The render frees it only when it ends, as every reference cycle (0110). A loop
of calls of

```jinja
{% macro f() %}{% set lst = [] %}{% macro m() %}{{ lst|length }}{% endmacro %}{% set _ = lst.append(m) %}{% endmacro %}
```

grows by about 1.2 KB a call in a Debug build (22 MB at 10k calls, 60 MB at 40k); with a
body that reads no name it stays at 13 MB, since such a macro keeps no copy. Python's
generational collector frees the cycle during the render. A plain cycle made in a macro
(`{% set l = [] %}{% set _ = l.append(l) %}`) grows the same way, by less per call. 0375 removes
the case where the stored closure does not read the container (its copy then lacks it).

**Proposal.** Let the collector see a closure's keep-alive and the copy it leads to
(`ValueObject::Traverse` of a `Callable` reporting its `ClosureCapture`'s copy, which the
render holds through `KeepUntilEnd`), and run a walk during the render once the candidates and
kept copies pass a count that doubles after each walk, as `CycleCollector::Add` prunes. Measure
the walk's cost with `bench/count.py`: a render that keeps no closure must not pay.

**Done when.** The loop above stays in constant memory, the 0326 and 0373 rows still match,
and `bench/count.py` shows no regression.
