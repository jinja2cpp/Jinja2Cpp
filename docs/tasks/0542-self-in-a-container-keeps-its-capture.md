---
status: in-progress
pr:
priority: low
area: robustness
depends: [0529]
touches: [src/cycle_collector.h, src/cycle_collector.cpp, src/template_module.cpp, test/allocation_failure_test.cpp, test/template_module_test.cpp]
---
# A `self` inside a list or dict at an include's top level keeps every replaced tree

**Problem.** Found by the verifier of 0529. 0529 weakens a `self` a name of an included template's top level holds
(`{% set q = self %}`) when the include's names are copied for its kept closures, so the copy no longer keeps the
capture that keeps it. A `self` held inside a container (`{% set l = [self] %}`, `{% set d = {'s': self} %}`) is
copied as the container, whose `self` still keeps the capture: the same cycle. With `TemplateLookup::EveryUse`, no
cache and a source changing before every call, a `Template::MakeModule` macro that includes such a template keeps
every replaced tree: 3,000 calls peak at 31.3 MB RSS (Debug) against 13.5 MB for `{% set q = self %}`. Master
behaves the same: memory only, no safety issue, output matches Jinja2.

**Proposal.** Let the module's call collector see the copy (a cycle through the capture: copy -> container ->
`self` -> capture -> copy is collectable once nothing outside holds the capture), or weaken a `self` found inside
the containers the copy holds, with a bound on the walk.

**Done when** `ModuleCallTest.EveryUseStoredSelfLetGoOfStaysFlat` has the list and dict rows and they stay flat, clean
under address+undefined.

**Findings (batch 11 A, 2026-11-20).** The second option was tried and is not sound. Rebuilding a list or dict of the
include's copy with a weakened `self` inside (only when the frame's names alone hold it, aliasing kept, bounded walk)
keeps the five list, dict and nested rows flat (+31,000 to +43,000 live allocations over 1,000 calls before). It has
no safe read side, though: a call that takes the `self` out of the rebuilt container and stores it
(`{% set ns.s = l[0] %}`) keeps a `self` that keeps nothing. Once the closure that held the include's capture is gone,
`ns.s.b()` renders `B1` where Jinja2 and master render `B1T1`. Strengthening on read would rebuild the container on
every macro call (`CopyCaptured` runs per call), at a per-element cost, and would break the sharing of the list by
reference (`l.append` in one call seen by the next). Mutating the shared container in place brings the cycle back.
What is left is the first option: let the module's call collector traverse the copy and the capture (a
`ClosureCapture` is not a traversed object today, and `Callable` keep-alives are not visited by `TraverseBoxed`), so
that copy -> container -> `self` -> capture -> copy is collected once nothing outside holds the capture. That is a
change to the cycle collector's object model, so it wants an architect plan first. The prototype is in the project files at
`0542/container-weakening-prototype.patch`.

**Resolution (option B, plan `0542/plan.md`, Ruslan 2026-11-23).** The leak is an ephemeron cycle: `KeptObjects`
holds the copy X while its owner, the capture C, lives, and C is held only by the `m_keepAlive` of the blocks of a
`self` inside a container X holds. `KeptObjects::ReleaseUnreached`, called when a module's outermost call returns
(before `ReleaseExpired`, whose swaps would move the call's new entries, and `PruneReplaced`), walks from the values of each copy kept since the last call (up to 16, and
of one older copy in turn, with a backoff of 1 to 64 calls) at most 256 objects and 4,096 items. When C's every reference comes from
closure boxes in that walk and none of them is held from outside (the module's live roots, or a count above the
walk's), the copy's entries go as if C had expired, and what only the copy reached is emptied and let go of as the
collector's walk does, so the trees go in the same call. Nothing a value reads is rebuilt: a `self` stored out of the
container (`ns.s = l[0]`) keeps C, and `l.append` sharing stays. References the call's own collector holds for its
walk (`NoteStore` candidates) do not count as outside. Limits: a copy whose walk passes 256 objects or 4,096 items, a frame with
more than 16 kept records, more than 16 such copies made by one call (the rest wait for the older-copy turn),
a call that starts while a release empties what it found (it skips the check), a copy whose entries a prune or `ReleaseExpired` separated (the group is found around one entry, not by a scan), or a cycle that runs through an inner capture's `outer` stays kept (conservative); a copy
let go of is freed up to 64 calls late, or about a quarter of the kept entries in calls when many are kept. 0386 (the same shape during a render) can call the check from `Keep`'s prune
point as a follow-up.
