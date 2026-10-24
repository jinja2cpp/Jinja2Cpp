---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/643
priority: low
area: parity
depends: [0209]
touches: [src/statements.cpp#RenderIncludedTemplate]
shares: []
---
# An included template that extends a parent reading `loop`

Found while measuring 0209. `RenderIncludedTemplate` hides the including loop's `loop` from a
template only when that template names `loop` itself (a flag the parser sets for a read of `loop` outside the template's own loops). A template
that names nothing but extends a parent which reads `loop` is rendered with `loop` hidden, where
Jinja2 shows it when the loop's body names `loop`. Asking the parent on every include cost
+0.66% instructions on Render/inheritance, so 0209 left it.

**Proposal.** Resolve once per template, when `extends` runs, whether the parent chain names `loop`
(a flag on the child's frame), and hide the loop after that, or decide before the include from a
flag the loader computes for the parent chain when the child is loaded.

**Done when.** `{% for x in 'a' %}{{ loop.index }}{% include 'child' %}{% endfor %}` with `child` =
`{% extends 'parent' %}` and `parent` = `{{ loop.index }}` matches Jinja2.

**Resolution.** The case was the other way round: a template that names nothing was rendered
with the including loop visible, so a parent that reads `loop` saw it where Jinja2 hides it
(the loop's body does not name `loop`). A template that extends now counts as one that may
read the including loop (TemplateRenderer::SetMentionsLoop), so the include hides it unless
the including body names `loop`. Cost: one lookup of `loop` per include of an extending
template from inside a loop.
