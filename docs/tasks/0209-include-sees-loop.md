---
status: open
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
