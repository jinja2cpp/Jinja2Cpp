---
status: open
priority: low
area: parity
touches: [src/statements.cpp, src/render_context.h]
---
# `self` leftovers after 0139

**Problem.** Since 0139 (lazy `self`), `self` resolves from the running template's frame and
names set in the template's own scopes win, as in Jinja2. Two differences remain, pinned by
`loader.self_block_ignores_top_level_set` and `loader.self_printed`:
- In Jinja2 every block function declares its own `self`, so a block sees the template even
  after a top-level `{% set self = 1 %}`; Jinja2C++ lets an unscoped block see the set name.
  A `set self` inside the block, and a macro's view of a top-level `set self`, already match.
- `{{ self }}` prints `<TemplateReference None>` (or the template name) in Jinja2, and a dict
  of block callables here.

- The body of a `{% call %}` to a macro imported with `from` sees the module's `self`
  (`{% call m() %}{{ self.b() }}{% endcall %}` renders the module's `b`); in Jinja2 it is
  the calling template's. Before 0139 this failed with an undefined `self`.

Related, not pinned (no corpus option for env globals): a global the template changes in
place (`g.append(1)`) is changed for the rest of the render only; Jinja2 changes the global
object itself, for every later render too.

**Proposal.** Give the frame a minimum depth for `self` that `BlockStatement::RenderBody`
raises to the block's own scope; give the `self` map adapter a Python repr of its own.

**Done when.** Both cases leave `test/parity/divergences/loader.txt`.
