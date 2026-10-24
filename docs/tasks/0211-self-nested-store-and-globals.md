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

Found by the 0207 verifier, not pinned:
- A `self` taken at the top level of a template that extends another lacks the parent-only
  blocks (`{% extends 'p.j2' %}{% set x = self %}{% block b %}{{ x.c() }}{% endblock %}` with
  `c` only in `p.j2` fails; Jinja2 renders it): it is made before RenderAsParent pushes the
  parent's blocks. An ended include that extends has the same gap.
- A module that extends and sets `self` (`{% extends 'p.j2' %}{% set self = 5 %}`) exports
  nothing for `m.self`; Jinja2 exports 5. RenderAsParent erases `self` from the shared scope.
- `cycler()` is iterable in Jinja2 (`__iter__`); `cycler() is iterable` is False here.

Carried from 0143 and 0207, not pinned (no corpus option for env globals): a global the
template changes in place (`g.append(1)`) is changed for the rest of the render only; Jinja2
changes the global object itself, for every later render too. A render started inside it (a
user callable rendering another template of the same environment) shares the snapshot, so it
sees the change, and the outer render sees the inner one's.

**Proposal.** The parser marks a block whose body stores `self` only in nested scopes
(`with`, `for`, macro params are not nested for this); for such a block FindSelf skips the
block's own depth and reads the template's top level. The globals item needs a decision on
whether a render may change environment globals at all.

**Decision (Ruslan, 2026-10-24).** A render does not change environment globals. A global the
template changes in place is changed for that render (and renders nested inside it) only. This
is a deliberate divergence: Jinja2 lets one render leak state into every later render of the
environment, while each Jinja2C++ render works on its own snapshot of the globals.
Record it among the deliberate divergences in docs/parity.md (with 0235). This task's
remaining work is the `self` items above.

**Done when** no line of `test/parity/divergences/` names 0211.

**Resolution.** The parser records, per block and for the template's top level, whether a
read of `self` or a store of it comes first (NodeArena::SelfUse, in Jinja2's visit order:
`with` targets and macro parameters before their values and defaults); a body that stores
first has no `self` of its own and FindSelf reads the name as any other. RenderAsParent keeps
the `self` a child set at their shared top level for a parent that reads it from the context,
and puts it back after the parent for a module's export. The `self` of an extending
template's top level already had the parent-only blocks (0274); cases pin it. `cycler()` is
not iterable in Jinja2 either (Cycler has `__next__`, no `__iter__`), so nothing to do there.
