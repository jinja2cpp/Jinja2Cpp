---
status: open
priority: medium
area: robustness
depends: []
touches: [src/statements.cpp]
shares: []
---
# A loop frame the frame pool hands out leaks under the prompt_escape fuzzer

**Problem.** The scheduled one-hour fuzz run 42356910586 on master (2026-11-07, head 4b8ed5be,
job "fuzz prompt_escape") failed with LeakSanitizer: `1291 byte(s) leaked in 8 allocation(s)`,
`prompt_escape found 1 crashing or leaking input(s)`. The allocation's top frames are
`LoopFramePool::TakeOther(unsigned long)` (src/statements.cpp:1162), `LoopFramePool::Take`
(:1153) and `ForStatement::RenderLoopInSlots`. Later scheduled runs were green, so the input is
rare. It was found while triaging an unrelated CI failure of PR #815 (0357 S4); the leaking
input is in that run's `fuzz-artifacts-prompt_escape` artifact, which could not be downloaded
from the cloud runner.

**Proposal.** Download the artifact, minimise the input with the fuzzer (`-minimize_crash=1`),
replay it under `-DJINJA2CPP_WITH_SANITIZERS=address+undefined`, and find the path where a frame
`TakeOther` allocates is neither returned to the pool nor freed (likely an exception between
`Take` and the frame's release, or a frame kept by an escaping loop closure). Lifetimes are this
project's worst defect class (AGENTS.md), so check the fix against the cell protocol notes too.

**Done when.** The minimised input replays clean under LeakSanitizer and is added to
`fuzz/regressions/` (fuzz/README.md), with a unit test for the leaking path.

**Resolution.** The input came from the run's log (libFuzzer prints the reproducer):
`{% set ns = namespace(c=none) %}{% for x in [ns] %}{% set ns.c = loop.cyc2e %}{% endfor %}{{ ns.c('a', 'b') }}`.
The frame was not lost by the pool: it was held by a reference cycle the render's collector
never saw. `loop.cyc2e` is undefined, and `SetParentData` gives it an `Anchor` that keeps the
`loop` object, hence the frame, whose items hold `ns`. `NoteContainerStore`
(src/value_methods.cpp) noted a store only when the stored value was a list, dict, callable or
pair, so `set ns.c = <undefined>` (or `l.append(...)`) closed a cycle without a note. It now
also notes a store of any value whose object is traversable, which is what an anchor is. The
fix is in src/value_methods.cpp, not src/statements.cpp. Regression input
`fuzz/regressions/cycle-through-undefined-loop-attribute.j2` (leaks before the fix under
LeakSanitizer) and two rows of `ExpressionTest.CyclesThroughObjects`.
