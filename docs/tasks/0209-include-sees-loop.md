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

**Progress.** The premise above was too coarse. Jinja2 makes a `loop` object only when the loop's body (not
its iterable, filter or `else`) names `loop`, or the loop is recursive; an include then sees it, otherwise the
`loop` that resolves there is an enclosing extended loop's, a render parameter or none. `ForStatement` records
whether its body names `loop` (`SetBodyReadsLoop`, counted by `NodeArena::NoteLoopRead`), and
`RenderIncludedTemplate` takes the loop's binding out of sight for the length of the include when it does not,
or shows the next visible `loop` instead. Only templates that contain the name `loop` pay for the check.
Cases `statements.loop_not_visible_in_include` (line gone from the allow-list) and `statements.loop_include_*`.
