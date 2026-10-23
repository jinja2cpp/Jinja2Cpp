---
status: open
priority: low
area: parity
depends: [0209]
touches: [src/statements.cpp#RenderIncludedTemplate]
shares: []
---
# An include keeps a `loop` that a macro received as an argument

Found by Codex on the 0209 PR. 0209 hides the `loop` of an including loop from an included
template by testing whether the value found is a loop object (`LoopAccessor`). A macro
parameter named `loop` that receives a loop object is such a value too, so
`{% macro m(loop) %}{% include 't' %}{% endmacro %}{% for x in [1, 2] %}{{ m(loop) }}{% endfor %}`
with `t` = `{{ loop.index }}` renders undefined where Jinja2 renders `12`: the include there sees the
macro's local `loop`.

**Proposal.** Hide only the binding the `for` statement made (its slot or scope entry), not every
loop object, for example by marking the loop's own binding or by comparing the found value's state
against the innermost running loop of this context.

**Done when.** A parity case with the macro above matches Jinja2.
