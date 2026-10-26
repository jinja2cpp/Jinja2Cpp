---
status: open
priority: medium
area: robustness
touches: [src/template_env.cpp, test/user_callable_test.cpp]
---
# Changing a global can race with a render that just dropped the globals map

**Problem.** Found by the 0318 verifier under TSan. `ChangeGlobals` (`src/template_env.cpp`)
decides whether to edit the globals map in place by reading `globalValues.use_count()`. That
read is relaxed: when it sees 1 because a render on another thread just released its snapshot,
nothing orders that render's earlier reads of the map before the edit. TSan reports a data race
between `AddGlobal` and a render's snapshot conversion. With a real interleaving the render may
read a map that is being modified.

**Done when.** The in-place path synchronizes with the last release, for example with
`std::atomic_thread_fence(std::memory_order_acquire)` after `use_count() == 1`, or the map is
always replaced, measured with `bench/count.py`. A TSan row runs renders on two threads while a
third replaces a list global, and is clean.

**Limits.** Only the decision to edit in place is affected. Holders that are still alive keep
their own map, because a count above 1 copies it.
