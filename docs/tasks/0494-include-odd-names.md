---
status: open
priority: low
area: parity
depends: [0458]
touches: [src/statements.cpp#RenderByNames]
shares: [python/tests/test_errors.py]
---
# Include names Jinja2 rejects with other classes

**Problem.** Found by the 0458 verifier (Jinja2 3.1.6). The engine selects from any iterable an
include gets, where Jinja2's `select_template` fails on some first:

- `{% include {'a': 1} %}`: Jinja2 raises KeyError `-1` (TemplatesNotFound indexes `names[-1]`); the
  engine tries the key `a`, renders it when found and raises TemplatesNotFound otherwise.
- `{% include [[]] %}`: TypeError `unhashable type: 'list'`; here TemplatesNotFound.
- A Python set as the name: TypeError `'set' object is not subscriptable`; here TemplatesNotFound, or
  nothing under `ignore missing`.

**Proposal.** Low value. Fail a name that is a list or a dict inside the list with TypeError
(`unhashable type`), and decide whether to follow Jinja2's accidents for a mapping and a set or record
them as deliberate.

**Done when.** The three cases raise Jinja2's classes through the module, or the difference is recorded.
