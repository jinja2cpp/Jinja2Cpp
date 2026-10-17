---
status: open
priority: low
area: parity
depends: [0207]
touches: [src/statements.cpp#FindSelf, src/template_parser.cpp]
shares: [test/parity/divergences/loader.txt]
---
# `self` stored in a nested scope of a block; globals changed in place

Left by 0207.

**Problem.** Pinned in `test/parity/divergences/loader.txt`:
- `loader.self_block_with_self_reads_top_level`, `loader.self_block_for_self_reads_top_level`:
  `{% set self = 1 %}{% block b %}{% with self = 4 %}{% endwith %}{{ self is number }}{% endblock %}`
  (or `{% for self in [4] %}` in the block) gives False; Jinja2 gives True. Jinja2's block
  function declares `self` only when `find_undeclared` reports it for the body, and a store in
  a nested scope hides the name from that visitor, so the block's top level resolves `self` from
  the context, where the template's top-level `set` put 1. Jinja2C++ always gives a block its
  own `self` (RenderContext::SetSelfDepth).

Carried from 0143 and 0207, not pinned (no corpus option for env globals): a global the
template changes in place (`g.append(1)`) is changed for the rest of the render only; Jinja2
changes the global object itself, for every later render too. A render started inside it (a
user callable rendering another template of the same environment) shares the snapshot, so it
sees the change, and the outer render sees the inner one's.

**Proposal.** The parser marks a block whose body stores `self` only in nested scopes
(`with`, `for`, macro params are not nested for this); for such a block FindSelf skips the
block's own depth and reads the template's top level. The globals item needs a decision on
whether a render may change environment globals at all; it is likely to stay deliberate.

**Done when** no line of `test/parity/divergences/` names 0209.
