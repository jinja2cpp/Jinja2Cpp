---
status: done
priority: low
area: robustness
depends: [0251, 0390]
touches: [src/statements.cpp#LoopAccessor]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/844
---
# A kept loop over `loop` leaks through a cycle the collector cannot see

**Problem.** Found by the 0390 verifier under LeakSanitizer, and the same on master before
0390. `LoopAccessor::GetIteration` makes a generator whose lambda captures the iterated
loop's state and a `loop` object that owns it. The cycle collector (0251) cannot see inside
the lambda, so a cycle through it is never freed:

```
{% set ns = namespace(k=none) %}{% for x in [ns, ns, ns] %}{% for a in loop %}{% set ns.k = loop %}{% break %}{% endfor %}{% endfor %}ok
```

`ns` holds the inner loop, whose enumerator (and, since 0390, its `indexedList`) holds the
generator, which holds the outer loop, whose items hold `ns`. The render prints `ok`; the
loop states and the list leak (four such probes in the verifier run, all on master too).

**Proposal.** Make the generator's captured `loop` visible to the collector: an accessor
type for the iteration that `Traverse`s its loop (in place of `ListAdapter::CreateAdapter`
with a lambda), or have `LoopFrame::Traverse` report the iterated loop it keeps in
`lengthOf` and `indexedList` when its enumerator runs over a loop. Add an LSan-checked test
(the alloc or fuzz-regression suite runs under the sanitizer job).

**Done when.** The template above renders without a leak under
`-DJINJA2CPP_WITH_SANITIZERS=address+undefined` with `detect_leaks=1`.

**Resolution.** `LoopAccessor::GetIteration` makes its generator with
`ListAdapter::CreateGenerator`, which keeps the loop object (the frame's own accessor, owned by
the frame) in the list and `Traverse`s it, and makes each (item, loop) pair when it is read rather
than keeping it in the enumerator, which the collector does not see. So the collector follows the
kept inner loop through its `indexedList` to the outer loop. The generator lives in
internal_value.cpp: as a final class in statements.cpp GCC speculatively devirtualized the loop
enumerator calls in `RenderLoopInSlots` into it (+4% instructions on mitsuhiko_table). Rows in
`ExpressionTest.CyclesThroughObjects`, which the sanitizer job runs with leak detection.
