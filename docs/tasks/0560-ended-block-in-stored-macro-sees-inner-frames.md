---
status: open
pr:
priority: low
area: parity
depends: [0552]
touches: [src/statements.cpp#EnterEndedTemplateFrame]
shares: [test/borrowed_cells_test.cpp]
---
# A block in a macro stored past its render sees the names of the frames around the macro

**Problem.** Found by the verifier of 0552. Since 0552 a block inside a macro that a nested render stored renders once
that render has ended, from the template's ended blocks. Jinja2's block function sees only the template's top-level
context; ours sees the copy of every frame the macro kept, so names of a macro or a block around the stored macro show
through. With the outer `{{ inner() }}{{ g[-1]() }}` (BorrowedCellsSingle.NestedRenderValueOutlivesItsTemplate):

| nested template | Jinja2 3.1.6 | Jinja2C++ |
|---|---|---|
| `{% set x='X' %}{% macro o() %}{% set y='Y' %}{% macro m() %}{% block b %}[{{x}}{{y}}]{% endblock %}{% endmacro %}{% set _ = g.append(m) %}{% endmacro %}{{ o() }}` | `[X]` | `[XY]` |
| `{% block a %}{% set x='X' %}{% macro m() %}{% block b %}[{{x}}]{% endblock %}{% endmacro %}{% set _ = g.append(m) %}{% endblock %}` | `[]` | `[X]` |

The same under `{% extends %}` with the child overriding `a` gives `[]` against `[X]`. Before 0552 these printed
nothing, as every block in a stored macro did. Rendered without storing the macro, both match Python.

A second gap, found by the security review of #967: when the nested render fails after it stored the macro (the
host swallowing the error), the block renders nothing. `TemplateRenderer::KeepEndedSelf` runs on the success path of
`RenderBody` only, so the capture never gets `endedSelf`. With autoescape on, the inner template
`{% set x = "X<" %}{% for a in [[1],2] recursive %}{% if a is iterable %}{{ loop(a) }}{% else %}{% macro m() %}{% block b %}{{ x }}{% endblock %}[{{ x }}{{ a }}]{% endmacro %}{% set _ = g.append(m) %}{{ 1 // 0 }}{% endif %}{% endfor %}`
and the outer `{{ inner() }}{{ g[0]() }}` print `X&lt;[X&lt;1]` in Jinja2 3.1.6 and `[X&lt;1]` here
(BorrowedCellsSingle.NestedRenderThatFailsKeepsItsClosures). Memory-safe.

**Proposal.** Give the ended frame of EnterEndedTemplateFrame a base depth that hides the macro's kept frames above the
template's top level: an unscoped block then sees the copy of the top level only (the capture that holds `endedSelf`,
not the inner captures chained above it).

For the failure, keep the ended `self` on the failure path too (where ClosesOverOnFailure closes over the top level).

**Done when.** Rows with the templates above print Python's output in `NestedRenderValueOutlivesItsTemplate`, narrow
and wide, ASan clean.
