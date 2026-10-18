---
status: open
priority: medium
area: release
depends: [0221]
touches: []
---
# Close GitHub issues that master already fixes, before 2.0

**Problem.** The 0221 scout (2026-10-18) read all 49 open issues and milestones 3
("Release 1.2") and 7 ("Release 1.3"). About 35 open issues are fixed on master, mostly by
parity tasks 0012-0030 and the fuzzing work (0003): for example the crash reports #264,
#268, #252, #287, #288 and the parity reports #261-#267, #273, #290. The tracker still shows
them as open bugs, and both milestones are open. Nothing in the registry or the release
gates asks for the tracker to reflect master. External PRs #284 (CMake typo, still
applicable), #199 (dead `LogicalExprTest` fixture, idea still valid, diff stale) and #197
(Conan RapidJSON target, obsolete since the Conan 2 rewrite) are untouched.

**Proposal.** Before the RC, per issue: re-run its repro on the candidate, close it with a
comment naming the task/PR and "fixed in 2.0.0"; close obsolete ones (#61 `flip`, #85,
#195, #204) as not planned with the reason. Retarget or close milestones 1.2 and 1.3. Land
#284's typo fix and a fix for #199's dead fixture (instantiate a truthiness subset or delete
the fixture) as our commits crediting the authors, and close #197 with thanks. Closing
issues and PRs is an outward action: Ruslan approves the list before anything is closed.

**Done when.** Every open issue either names an open task or is closed with a pointer to
its fix; milestones 1.2 and 1.3 are closed; #284/#199/#197 are resolved; the 0221 RC
checklist has this as a step.
