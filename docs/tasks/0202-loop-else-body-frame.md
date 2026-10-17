---
status: done
priority: low
area: parity
depends: [0168]
touches: [src/template_parser.cpp#ParseElse, src/statements.cpp#ForStatement]
shares: [src/name_resolver.cpp, test/parity/cases/statements.py]
---
# A loop's `else` body is a frame of its own

**Problem.** Jinja2 runs a `for` loop's `else` body in a frame of its own: a `set` there
does not leak, and a read there of a name the enclosing frame stores later is a read from
an inner frame, so it sees the frame's entry binding (0168). Jinja2C++ runs the `else` body
in the enclosing scope and the resolver counts it as the enclosing frame:
`{% for j in [] %}{% else %}{% set x = 1 %}{% endfor %}{% for k in [1] %}{{ x }}{% endfor %}`
with `x=3` prints `1` (Jinja2 `3`), and
`{% for i in [1] %}{% for j in e %}{% else %}{{ x }}{% endfor %}{% set x = 9 %}{% endfor %}`
with `e=[]` prints `3` (Jinja2 nothing): `statements.loop_else_set_is_local` and
`statements.idtracking_loop_else_read_before_set`.

**Proposal.** Render the `else` body in a scope of its own and give it a resolver frame
(`PushWith` under the loop's parent, as block `set` bodies have since 0168); the slots of the
names read after it stay right once the store no longer leaks.

**Done when.** No divergences line names 0202.

**Done.** `ParseElse` gives a loop's `else` a `With` frame under the loop's parent; its
statements are not in a branch (`IsLoopElse`), so it binds names at entry like any frame,
and `ForStatement::RenderElse` runs the body in a scope of its own. Cases:
`statements.idtracking_loop_else_binds_at_entry`, `loop_else_set_keeps_outer_target`.
