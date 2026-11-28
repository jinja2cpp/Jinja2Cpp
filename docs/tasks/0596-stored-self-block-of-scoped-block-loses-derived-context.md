---
status: open
priority: low
area: parity
depends: [0584]
touches: [src/statements.cpp#SelfBlockCall, src/statements.cpp#SuperBlockCall, src/statements.cpp#FindSelf, src/statements.cpp#BlockStatement, src/render_context.h, src/template_parser.cpp, src/expression_parser.cpp, src/node_arena.h]
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

**Progress (batch 15 A).** The scoped block call's own context owns the capture of the scopes its derived context
holds (RenderContext::OwnsDerivedCapture) for the `self` and super() made in it, and copies them with the template's
top level when the call ends; closures defined in a scoped block copy the top level too. super() is captured only when
the body names `super` (BlockStatement::SetReadsSuper). 14 corpus cases; the deliberate 0165 line
`loader.self_of_ended_scoped_block_loses_loop` went as it now matches. Residual shapes: 0607.
