---
status: open
priority: low
area: parity
depends: [0198]
touches: [src/statements.cpp#MakeTemplateSelf, src/statements.cpp#ImportStatement]
shares: [test/parity/divergences/loader.txt]
---
# `self` cases left after 0143 and 0198

**Problem.** Pinned in `test/parity/divergences/loader.txt`:
- `loader.self_of_ended_include`: a `self` an include stored (`{% set ns.s = self %}`) and
  called after the include ended renders the caller's block of that name (`OB`); Jinja2
  renders the include's (`IB`). The include's `TemplateFrame` and block stack are locals of
  its render, so nothing of them is left to find.
- `loader.self_scoped_block_calls_scoped_block_in_loop`: `self.c()` inside a scoped block in
  a loop renders `c` with the top-level names; Jinja2 passes the calling block's context,
  which has the loop locals, so a scoped `c` sees `v`.
- `loader.module_set_self_exported`: a module whose top level does `{% set self = 5 %}`
  exports nothing for `l.self`; Jinja2 exports 5 (the import skips the name `self`).

Not pinned, found by the 0143/0198 verifier:
- `{% set self = 1 %}{% block b %}{% with self = 4 %}{% endwith %}{{ self is number }}{% endblock %}`
  (or `{% for self in [4] %}` in the block) gives False; Jinja2 gives True: a block that
  stores `self` in a nested scope does not declare its own, so its top level resolves `self`
  from the context.
- `self is iterable` is False; Jinja2's TemplateReference has `__getitem__`, so True.

Carried from 0143, not pinned (no corpus option for env globals): a global the template
changes in place (`g.append(1)`) is changed for the rest of the render only; Jinja2 changes
the global object itself, for every later render too. A render started inside it (a user
callable rendering another template of the same environment) shares the snapshot, so it sees
the change, and the outer render sees the inner one's.

**Proposal.** Keep an include's block stack alive with its `self` (shared ownership from the
`self` adapter, or a per-render list of stacks as for modules) and give the include's frame
the same lifetime; for scoped blocks, let the block callable link to the caller's scopes when
the caller is a scoped block of the same template. The export: drop the `self` exclusion
when the module's own top level set the name.

**Done when** no line of `test/parity/divergences/` names 0206.
