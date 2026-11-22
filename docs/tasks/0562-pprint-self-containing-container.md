---
status: in-progress
priority: low
area: parity
depends: [0545]
touches: [src/serialize_filters.cpp#Pprint]
shares: [test/parity/cases/filters.py]
---
# pprint of a list or dict that contains itself fails with a recursion error

**Problem.** `{% set l = [] %}{% do l.append(l) %}{{ l | pprint }}` and
`{% set d = {} %}{% do d.update({'self': d}) %}{{ d | pprint }}` fail with "Maximum recursion depth exceeded" in the
engine. Python's pprint prints `[<Recursion on list with id=...>]` and `{'self': <Recursion on dict with id=...>}`
(Jinja2 3.1.6, CPython 3.12). `ReprWidthAtMost`, which measures a value for pprint's line breaks, has no
recursion guard (found by the 0545 verifier; pre-existing on c25c2ff). 0545 gave the wide path a container stack for
mappingproxies and views only.

**Proposal.** Give the measuring pass the container stack the writer has, and write Python's
`<Recursion on TYPE with id=N>` for a container met again. The id is an address, so the corpus case compares the text
before it (as `operators.mapping_proxy_*` recursion cases do).

**Done when** both templates render with Python's text up to the id, in narrow and wide runs.

**Result.** The printer, its measuring pass and the one-line repr share one context: a dict, list or tuple met
again inside itself prints and measures as `<Recursion on TYPE with id=N>`, and the dict a mappingproxy prints is
not on it (Python prints a copy). The single-pass-list check before printing walks each container once. Cases
`filters.pprint_self_*` and `filters.pprint_shared_not_recursive`.
