---
status: open
priority: low
area: robustness
depends: [0353]
touches: [python/src/environment.cpp#Keep]
shares: [python/tests/test_lifetime.py, docs/python.md]
---
# Bound the callables an Environment keeps alive

**Problem.** `PyEnvironment::Keep` holds every filter, test, `finalize`, autoescape callable and
loader the environment was ever given, for its whole life, because templates loaded earlier
hold borrowed pointers to the callables they were compiled with
(`test_filter_added_after_load_binds_at_next_load` relies on that). Replacing `env.filters`
entries in a loop grows the environment without bound (0353 verifier N7, in docs/python.md
"Differences").

**Proposal.** Give each loaded template strong references to the callables it was compiled
with, visible to the garbage collector through `tp_traverse` of the template wrapper or of a
shared callable set, so the environment can drop an entry once it is replaced. This is a
lifetime change: it needs an ASan run of the pytest suite and a stress case that replaces a
filter 10k times while old templates still render.

**Acceptance.** Memory stays flat in the stress case; templates loaded before a replacement
still render with their old callables; ASan clean.
