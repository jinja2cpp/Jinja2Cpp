---
status: done
priority: low
area: parity
depends: []
touches: [src/statements.cpp#TemplateRenderer]
shares: [test/parity/divergences/loader.txt]
---
# Blocks called through `self` at a child's top level lack the parent's blocks

Found by the 0181 verifier. Pinned by `loader.self_block_after_extends_at_top_level`:
`{% extends 'a.txt' %}{% block b1 %}C{{ super() }}{% endblock %}{{ self.b1() }}` with
`a.txt` = `{% block b1 %}A{% endblock %}` fails with "there is no parent block called 'b1'";
Jinja2 renders `CA` (the child's top-level output after `extends` is dropped, but it runs).

**Cause.** A child's top level runs before TemplateRenderer::RenderAsParent pushes the
parent's blocks onto the stack, so `super()` in a block rendered from there finds none.
Jinja2 adds a parent's blocks to `context.blocks` when `extends` runs. 0211 lists the same
gap for a `self` stored at the top level and called later.

**Done when.** The case renders `CA` and its line leaves `test/parity/divergences/loader.txt`.

**Progress.** `ExtendsStatement::Render` pushes the parent's blocks onto the child's stack when `extends`
runs (`TemplateFrame::extendable`), as Jinja2 adds `parent_template.blocks` to `context.blocks`; `RenderAsParent`
no longer pushes them. A grandparent's blocks are pushed when the parent's own `extends` runs, as in Jinja2.
Cases `loader.self_block_after_extends_set`, `_parent_only_block` and `self_block_before_extends` observe the
value through a `set`, which Python runs after `extends`. Python skips the *output* nodes there, so
`{{ self.b1() }}` as an output is never evaluated; Jinja2C++ still evaluates and drops it (task 0203).
