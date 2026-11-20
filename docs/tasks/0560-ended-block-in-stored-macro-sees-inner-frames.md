---
status: open
pr:
priority: low
area: parity
depends: [0552]
touches: [src/statements.cpp#TemplateRenderer::RenderBody]
shares: [test/borrowed_cells_test.cpp]
---
# A block in a macro stored by a nested render that failed renders nothing

**Problem.** Found by the security review of #967 (0552). Since 0552 a block inside a macro that a nested render stored
renders from the template's ended blocks once that render has ended. When the nested render fails after it stored the
macro (the host swallowing the error), the block renders nothing: `TemplateRenderer::KeepEndedSelf` runs on the
success path of `RenderBody` only, so the capture never gets `endedSelf`. With autoescape on, the inner template
`{% set x = "X<" %}{% for a in [[1],2] recursive %}{% if a is iterable %}{{ loop(a) }}{% else %}{% macro m() %}{% block b %}{{ x }}{% endblock %}[{{ x }}{{ a }}]{% endmacro %}{% set _ = g.append(m) %}{{ 1 // 0 }}{% endif %}{% endfor %}`
and the outer `{{ inner() }}{{ g[0]() }}` print `X&lt;[X&lt;1]` in Jinja2 3.1.6 and `[X&lt;1]` here. Memory-safe.

(The other gap first filed here, a block in such a macro seeing the names of the frames around the macro, was fixed in
#967 after Codex review: the ended frame's top level is a scope of the template's top-level copy only.)

**Proposal.** Keep the ended `self` on the failure path too, where ClosesOverOnFailure closes over the template's top
level (`KeepEndedSelf` needs the scopes the failure left above the base to be ended first).

**Done when.** A `BorrowedCellsSingle.NestedRenderThatFailsKeepsItsClosures` row with the templates above prints
Python's output, narrow and wide, ASan clean.
