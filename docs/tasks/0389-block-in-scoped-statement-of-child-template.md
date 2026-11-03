---
status: open
priority: low
area: parity
depends: []
touches: [src/template_parser.h, src/statements.cpp, test/parity/divergences/loader.txt]
---
# A block in a scoped statement of a child template also renders in place

**Problem.** Found by the 0369 verifier. In a template that extends another, Jinja2 3.1.6 drops
output outside blocks, and a `{% block %}` at the top level only defines the block. A block
nested in `{% autoescape %}` or `{% with %}` (statements that open a scope but no new frame
level for `CodeGenerator.visit_Block`'s `frame.toplevel` check) is rendered at its position as
well: with `base.j2` = `<{% block head %}H{% endblock %}|{% block body %}B{% endblock %}>`,
`{% extends 'base.j2' %}{% with %}{% block body %}b{% endblock %}{% endwith %}` renders
`b<H|b>` in Python and `<H|b>` in Jinja2C++. Text next to the block (`<i>` inside the
autoescape) is still dropped, and a block in an `{% if %}` is not rendered in place, in both.
Cases: `loader.extends_block_in_scoped_statement`, `loader.extends_block_in_with`.

**Proposal.** Trace which statements make `frame.toplevel` false in Jinja2's compiler
(`visit_Block`: `if frame.toplevel: if self.has_known_extends: return`), and let the parser's
output check after `{% extends %}` (`OutputCheckAfterExtends`) keep a block call in place under
those statements, guarded as Jinja2 guards it (`if parent_template is None` when the extends is
not known at that point).

**Done when** both divergence lines are deleted and `ctest -R parity` passes.
