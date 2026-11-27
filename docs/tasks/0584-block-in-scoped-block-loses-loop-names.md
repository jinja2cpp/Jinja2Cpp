---
status: open
priority: low
area: parity
depends: []
touches: [src/statements.cpp#BlockStatement::Render]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt]
---
# An unscoped block nested in a scoped block does not see the loop around them

**Problem.** `{% for i in [1] %}{% block x scoped %}{% block y %}[{{ i }}]{% endblock %}{% endblock %}{% endfor %}`
renders `[1]` in Jinja2 3.1.6 and `[]` in Jinja2C++. Jinja2 calls `y` with the context `x` got, which
`context.derived(locals())` made with the loop's `i` in it; the engine renders the unscoped `y` on the scopes its
template's top level sees (TemplateFrame::baseDepth), which leave the loop out. Found by the architect of 0558
(plan /mnt/project-files/0558/plan.md, section 8). Pinned by the corpus case
`statements.block_in_scoped_block_reads_loop` (divergence line in `divergences/statements.txt`).

The same holds for an `{% include %}` in a scoped block in a loop: Jinja2's included template has the loop's
`loop` in its `Context.parent`, the engine's does not (found by the 0558 verifier).

**Proposal.** An unscoped block called inside a scoped block's body sees what that scoped block sees (the depth
0558 records as `RenderContext::m_derivedDepth` is a natural bound); check a child template's override of `y`, and
`self.y()` and `super()` there, against Jinja2.

**Done when.** The corpus case matches and its divergence line is gone.
