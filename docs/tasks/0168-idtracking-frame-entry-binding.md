---
status: open
priority: low
area: parity
depends: [0165]
touches: [src/name_resolver.cpp]
shares: [src/name_resolver.h, test/parity/cases/statements.py]
---
# Bind a name the frame assigns at frame entry (Jinja2 idtracking)

**Problem.** Jinja2 decides per frame, at frame entry, what a name refers to: a name the
frame assigns anywhere (`set`, `macro`) is bound to the frame, and a read before the
assignment gives undefined, not the outer value. Jinja2C++ resolves an unbound name outward
on each read. `{% for i in [1,2] %}{% for j in [1] %}{{ x }}{% endfor %}{% set x = i %}{% endfor %}`
with `x=3` renders `''` in Python and `33` here
(`statements.idtracking_inner_read_before_outer_set`). The macro forms (a macro reading a
root name set after the call; a default reading one) are listed in
docs/design/0038-lexical-scoping-plan.md section 8 and land as lines under this task with
0165/0166.

**Proposal.** A per-name "loaded before stored / stored in a branch" bit computed at Load
by the name resolver, no runtime cost; 0117 P4's root slots should carry it. Kept out of
0038 by decision 3 of its plan.

**Done when.** No divergences line names 0168 and `ctest -R parity` passes.
