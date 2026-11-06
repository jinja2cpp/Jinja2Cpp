---
status: open
priority: low
area: parity
depends: [0353]
touches: [python/src/environment.cpp#MakeTemplate, python/src/environment.cpp#GetTemplate]
shares: [python/tests/test_render.py, docs/python.md]
---
# Templates cached before env.globals is replaced see the new dict

**Problem.** In Jinja2 a cached template, and a module imported from it, keeps the globals dict
it was made with: after `env.globals = {...}`, `get_template` of a cached name still sees the
old dict. The module makes a new `Template` on each `get_template` with the dict of that
moment, and an imported template sees the importer's (found by the 0353 verifier, in
docs/python.md "Differences").

**Proposal.** Store the globals dict with the cached engine template (the cache entry, not the
Python wrapper), and pass the imported template its own stored dict instead of the importer's.
Check against the design principle (host data immutable, renders independent): the dict is
captured, not shared mutably, so it fits.

**Acceptance.** pytest cases for replace-then-get_template and for import after replace match
Jinja2; the docs/python.md line is removed.
