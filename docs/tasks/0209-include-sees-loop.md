---
status: done
priority: low
area: parity
touches: [src/statements.cpp#RenderIncludedTemplate]
shares: [test/parity/divergences/statements.txt]
---
# A template included in a loop sees `loop`

Found by 0175. In Jinja2 an included template sees the including loop's target names
(`x`) but not `loop`: `{% for x in [1, 2] %}{% include 't' %}{% endfor %}` with `t` =
`{{ loop is defined }}{{ x }}` prints `False1False2`. Jinja2C++ clones the render
context with its scopes, so `loop` is defined there (`True1True2`).

Pinned by `statements.loop_not_visible_in_include`. Fix: hide the `loop` binding of the
innermost enclosing loop (slot or scope) from the cloned context in
`RenderIncludedTemplate` when it is rendered with context; check `{% import ... with context %}`
too.

**Progress.** `RenderIncludedTemplate` binds `loop` in the included template's scope to the next `loop`
that is not a loop object (a top-level `{% set loop %}` or a render parameter), or to undefined. Cases
`statements.loop_not_visible_in_include` (line gone from the allow-list) and `statements.loop_include_*`.
