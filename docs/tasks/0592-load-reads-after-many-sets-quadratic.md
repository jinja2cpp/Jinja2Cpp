---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1019
priority: low
area: robustness
depends: []
touches: [src/name_resolver.cpp]
shares: []
---
# Load: name reads after many `set`s in one body cost reads x sets

**Problem.** Loading a body with many distinct `{% set %}` names followed by many name reads is quadratic. Measured
through the Python module (Debug build, master behaviour, found by the 0558 verifier): `{% for x in [1] %}` with
30000 `{% set sN = 1 %}` then 10000 `{{ q }}` loads in 2.2 s, while the 30000 sets alone load in 0.1 s and 10000
reads with no sets in 0.03 s; the verifier's Release numbers were 0.127 s, 0.487 s and 2.03 s for 10k, 40k and 160k
reads after 30000 sets. A template of about 1 MB, which untrusted input can be, stalls the loader for seconds.
(0558 made calls cost the same as reads there: its own Seal check was made linear in #1014.)

**Proposal.** Profile the Load (likely the resolver's per-use search of a frame's stores, `ResolveSlots` or the
entry-binding pass) and index a frame's stores by name once, so that each read costs O(1) or O(log n).

**Done when.** The example above loads in time linear in its size (a test with a generous bound, or a
`bench/count.py` Load row), and the Load rows of the instruction-count gate do not regress.
