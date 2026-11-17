---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/936
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


**Resolution (batch 9 D).** Each engine environment of a `PyEnvironment` has a Python list of what it borrows
(`BoundObjects`: translations, loader, finalize, autoescape callable, chat clock, filters, tests), and every
template loaded from it holds that list (traversed: a list is a GC container, so references are counted once).
A load that finds a bound filter or test replaced or removed starts a new engine environment and list, so a
replaced callable goes with the last template that binds it. The translations, read from the environment at
each call, are item 0 of the current list and replaced in place by a new install; a borrowed callable holds
itself while it runs. Pinned by `test_replaced_filters_go_while_templates_loaded_before_keep_theirs` (10k
replacements, old templates and includes still render their filter) and `test_reinstalled_translations_go`,
`test_translations_reinstalled_by_their_own_call`. Side effect: a `get_template` after a filter is replaced reads
the loader again and gives a new Template (Jinja2 gives the cached one, which looks the new filter up at render).
