---
status: open
priority: low
area: api
depends: [0357]
touches: [src/value_visitors.h#InputValueConvertor]
shares: [test/public_host_object_test.cpp]
---
# A tuple a host returns stays a tuple

**Problem.** Found while implementing 0357 S1a. `GenericList::IsTuple()` (0357, Codex review) marks a
tuple the engine exports, but the way in ignores it: a `GenericList` whose accessor answers
`IsTuple()` true, returned by a `UserCallable` or passed as a parameter, becomes a plain list in the
engine (`InputValueConvertor`, `src/value_visitors.h`). So `{{ f() }}` prints `[1, 2]` where the host
meant `(1, 2)`, and nested inside a container it reprs as a list. Within one host call an exported tuple handed back is restored as the original, so only a
tuple the host builds itself is affected.

**Proposal.** Mark the `ListAdapter` as a tuple (`MarkAsTuple`) where a `GenericList` enters the engine
and its accessor says it is one. Ask only on the generic-list path (one virtual call where a list is
converted, none per item). Row in `test/public_host_object_test.cpp`: a callable returning a
`GenericList` with `IsTuple()` true prints as `(1, 2)`, and one element as `(1,)`.
