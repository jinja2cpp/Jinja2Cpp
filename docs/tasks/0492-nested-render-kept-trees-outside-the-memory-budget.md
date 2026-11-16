---
status: in-progress
priority: high
area: robustness
depends: [0481, 0459]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, src/template_impl.h]
---
# The trees an outer render keeps for its nested renders are outside the memory budget

**Problem.** Found by the 0481 verifier. Since 0481 the outermost render that shares its globals
with a render nested in it keeps the nested template, the templates it loaded and owned copies of
its params until it ends (`CycleCollector::KeepTree`), unconditionally: two versions that gave them
back when nothing could have escaped each had a use-after-free (0481 Resolution). A host callable
that loads a new `Template` per call, called in a loop, makes the outer render hold one tree per
call, and `maxRenderMemory` does not see them: 20000 nested renders of a 2 KB template took the
process from about 45 to 122 MB; the verifier's 100000 went from 390 to 1007 MB with
`maxRenderMemory` at the `UntrustedLimits` 256 MiB, and a 1 MiB budget failed the nested `Load`
after about 18000 renders (about 57 B of budget per kept template leaks into the nested render's
charge). The loop count is template text; the template per call is the host's pattern (templates
loaded by name from the environment's cache are one tree and kept once). Also: a nested render
that shares the snapshot copies its params (`OwnedArgument`) each time, O(size) per render, and
converts them twice.

A keeper nested in a template module's call hands its trees to the module's collector, which
lives as long as the module (0481 verifier round 2, L1).

**Proposal.** Charge a kept tree (its arena and source bytes) to the keeper's budget when it is
kept and release it when it goes, under the outer render's limits rather than the nested one's;
a keep past the budget fails with `ResourceLimitExceeded` like any other allocation. Find where
the 57 B per nested render go. To give trees back early, use ownership rather than reachability: a value that names a tree
(a macro's body, `caller`, loop recursion, `self`, a view of its text) holds a keep-alive when its
render has a keeper, or a per-tree count of live callables is checked when the nested render
ends; an architect plan first, and the perf gate. Copy params lazily on a store.

**Done when.** The loop above under `UntrustedLimits` with a 1 MiB budget fails with
`ResourceLimitExceeded` in the outer render, a budget that fits renders, and ASan stays clean.

**Resolution (charge, 2026-11-16).** A kept tree counts against the budget of the render that keeps it,
by its size (`TemplateImpl::TreeBytes`: the object, its sealed arena, the source and converted texts),
computed only for a tree that collector does not hold yet. That render is usually not the running one:
the nested render installed limits of its own, and the keeper's live state is the copy its
`RenderLimitsScope` saved. So each installed `RenderLimits` now records the state it interrupted
(`interrupted`) and the collector of the render that installed it (`collector`, read from
`CycleCollector::Current()` in `Install`); `detail::ChargeRenderMemoryOf` walks that chain to the
keeper's level.
- A nested render's own template and each template it loads (`CycleCollector::KeepNewTree`) are
  charged before they are kept or run. One that does not fit is not kept: the keeper's level is
  overdrawn, so the keeper fails with `maxRenderMemory` at its next step, and the nested render fails
  now with `ResourceLimitExceeded` ("a template ... does not fit in that render's budget"). So a
  loop in a render between the two (outer, then `middle()` looping over `inner()`) stops at the
  budget too, not only a loop in the keeper (verifier round 1).
- A nested keeper that hands its trees to its outer collector at its end (`Finish`) charges them there,
  under that render's limits, which are the running ones by then. That path keeps first and only
  overdraws: a throw would make the hand-over drop trees that values still name.
- A tree is charged once per collector (`treeSet`), so the environment's cached template is one
  charge. A keeper without a budget charges nothing, whatever the nested render's budget is: what
  the outer render keeps is its host's to bound. A render that runs inside no other one never keeps.

The 57 B per nested render: a host `Template::Load` inside a host callable runs under the outer
render's limits, so the tree's value objects are charged there, and the keep holds them until the
outer render ends. That charge is balanced by the credit when the tree goes (the 0459 rule), so it
stays; `TreeBytes` adds the arena and the text, which no allocation function counted.

Tests (`test/limits_test.cpp`): `KeptNestedTemplatesCountAgainstTheOuterBudget` (20000 nested renders
of a new 2 KB template under `UntrustedSettings` with a 1 MiB budget fail the outer render with
`maxRenderMemory`; on master they rendered), `WideKeptNestedTemplatesCountAgainstTheOuterBudget`,
`KeptTemplatesOfADeeperRenderCountAgainstTheOuterBudget` (the loop one level down: fewer than 1000 of 20000 `inner()` renders keep a tree, the rest fail with `maxRenderMemory`),
`KeptNestedTemplatesFitABudget` (20 fit) and `KeptCachedTemplateCountsOnce` (5000 renders of the
environment's cached template fit).

Not done here, filed as 0521: giving trees back before the keeper ends (ownership rather than
reachability, needs an architect plan) and copying a nested render's params lazily. A render nested
in a template module's call that reads the module's own globals has no keeper (`SharedSnapshotKeeper`
does not match them) and keeps nothing; what a module retains across calls is 0508's.
