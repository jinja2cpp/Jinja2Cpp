---
status: open
priority: low
area: parity
depends: [0584]
touches: [src/statements.cpp#SelfBlockCall, src/statements.cpp#SuperBlockCall]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt]
---
# A `self` block or `super` stored in a scoped block and called after the loop loses the derived context

**Problem.** Found by the verifier of batch 14 C (0584), identical on master c4561b9.
`{% set ns = namespace(f=none) %}{% for i in [1, 2] %}{% block x scoped %}{% set ns.f = self.y %}{% block y %}[{{ i }}]{% endblock %}{% endblock %}{% endfor %}|{{ ns.f() }}`
renders `[1][2]|[]` where Jinja2 3.1.6 renders `[1][2]|[2]`: Jinja2's `self` is a TemplateReference of the block's
derived context, so the stored `self.y` renders with the last pass's `i`. With `{{ loop.index }}` in `y` the engine
fails ('loop' is undefined) where Jinja2 prints `[11][22]|[22]`. A `super` a child override stores and the parent
calls after the loop prints `<>` where Jinja2 prints `<2>`. Pinned by the corpus cases
`statements.stored_self_block_of_scoped_block_after_loop` and `statements.stored_self_block_of_scoped_block_reads_loop`.

**Proposal.** `self` made in a scoped block (and `super` in a block called in it) keeps what the derived context held
when the loop's frame ends, as a closure defined in the block does (ClosureSite capture, 0373).

**Done when.** Both corpus cases match and their divergence lines are gone, narrow and wide.
