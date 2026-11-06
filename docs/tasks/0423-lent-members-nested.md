---
status: open
priority: low
area: api
depends: [0357]
touches: [src/internal_value.cpp#LendingMapView]
shares: [test/public_host_object_test.cpp]
---
# Template callables nested two levels deep in a host call's argument

**Problem.** Found while implementing 0357 S1a (plan section 7, inventory gap). A template callable
inside a container passed to a host call is now lent for the call: `read({'m': m})`,
`read([m])`, `read(namespace(fn=m))`, `read(loop)` (its `cycle`) can call it. Only the containers
the argument itself holds are wrapped (`LendingMapView`, `LendingListView`); whether a callable two
levels down (`read([{'k': m}])`, `read({'a': {'m': m}})`) is lent is not tested, and the
implementer reported it as unverified. Python passes the macro at any depth. A lent view a C++
callable keeps after the call degrades to `None` for its callable members rather than raising
`TemplateExpired`, which also differs from a top-level lent callable.

**Proposal.** Wrap containers read through a lending view as lending views too (the thread-local
frame is already there), so depth does not matter, and make a member read after the call raise
`TemplateExpired` like a top-level lent callable. Rows in `test/public_host_object_test.cpp` for
depth 2 and 3 and for a view kept past the call.
