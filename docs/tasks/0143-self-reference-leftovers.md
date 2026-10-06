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

- Unpinned cases the 0139 verifier found (none a regression; several crashed before):
  - `self` that escapes its template (`{% set s = self %}` in a module, read as `l.s.b()`, or
    stored in a `namespace()` by a macro) renders the running template's block or nothing;
    Jinja2 renders the module's (`MB`).
  - `self` passed to a macro imported without context renders the importer's block in the
    macro's context: the block does not see the importer's parameters or top-level sets
    (`{% block b %}[{{ p }}]{% endblock %}{{ l.show(self) }}` gives `[]` for `[1]`).
  - `self.b()` in a `{% call l.m() %}` body (`import ... as`) renders nothing (Jinja2: `B`).
  - A module that sets `self` at its top level exports `{}` (Jinja2: the value), and a module
    that extends another reports its parent's blocks as undefined through `self`.
  - `self.b(1, 2, x=3)` is accepted; Jinja2 raises TypeError.

Related, not pinned (no corpus option for env globals): a global the template changes in
place (`g.append(1)`) is changed for the rest of the render only; Jinja2 changes the global
object itself, for every later render too. A render started inside it (a user callable
rendering another template of the same environment) shares the snapshot, so it sees the
change, and the outer render sees the inner one's.

**Proposal.** Give the frame a minimum depth for `self` that `BlockStatement::RenderBody`
raises to the block's own scope; give the `self` map adapter a Python repr of its own.

**Done when.** Both cases leave `test/parity/divergences/loader.txt`.
