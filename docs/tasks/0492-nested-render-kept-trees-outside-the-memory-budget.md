---
status: open
priority: medium
area: robustness
depends: [0481, 0459]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, src/template_impl.h]
---
# The trees an outer render keeps for its nested renders are outside the memory budget

**Problem.** Found by the 0481 verifier. Since 0481 the outermost render that shares its globals
with a render nested in it keeps the nested template, the templates it loaded and owned copies of
its params until it ends (`CycleCollector::KeepTree`), once a global was written in place
(`DropTreesUnlessWritten`). A host callable that loads a new `Template` per call, called in a
loop by a template that also appended to a global, makes the outer render hold one tree per call,
and `maxRenderMemory` does not see them: 20000 nested renders of a 2 KB template took the process
from 47 to 122 MB; the verifier's 100000 went from 390 to 1007 MB with `maxRenderMemory` at the
`UntrustedLimits` 256 MiB, and a 1 MiB budget failed the nested `Load` after about 18000 renders
(about 57 B of budget per kept template leaks into the nested render's charge). The loop count is
template text; the template per call is the host's pattern. Also: a nested render that shares the
snapshot copies its params (`OwnedArgument`) each time, O(size) per render, even when it stores
nothing in a global.

**Proposal.** Charge a kept tree (its arena and source bytes) to the keeper's budget when it is
kept and release it when it goes, under the outer render's limits rather than the nested one's;
a keep past the budget fails with `ResourceLimitExceeded` like any other allocation. Find where
the 57 B per nested render go. Optionally keep only what the nested render could have stored:
mark a write of a global's cell in the keeper's registry during the nested render (also through
an adapter the outer render linked), and copy params lazily on a store.

**Done when.** The loop above under `UntrustedLimits` with a 1 MiB budget fails with
`ResourceLimitExceeded` in the outer render, a budget that fits renders, and ASan stays clean.
