---
status: done
priority: low
area: parity
depends: [0167]
touches: [src/statements.cpp#MakeTemplateSelf]
shares: [test/parity/divergences/loader.txt]
---
# A block called through `self` runs in its own template

**Problem.** `self.b()` renders block `b` with the context of whoever calls it. Passed to a
macro of an imported module, the block reads the module's names instead of its own
template's: `{% set v = 'main' %}{% block b %}[{{ v }}]{% endblock %}{% import 'm' as l %}{{ l.n(self) }}`
with `m` = `{% set v = 'mod' %}{% macro n(s) %}{{ s.b() }}{% endmacro %}` gives
`[main][mod]`; Jinja2 gives `[main][main]` (`loader.importer_block_via_self_in_imported_macro`).
`MakeTemplateSelf` picks the frame that owns the blocks but builds the block's context from
the caller's (`RenderContext blockContext(context, frame->baseDepth)`). Before 0167 the
module shared the importer's context, so the case looked right by accident only when the
module set nothing of the same name. Found by the 0167 verifier.

**Proposal.** Give each `self` callable a `ClosureSite` of the template's top-level scope and
run the block through `HomeCall`, as macro bodies do (0165), so the block resolves names
where it is defined. Keep the frame search for blocks overridden by a child template.

**Done when** the corpus case matches and its divergence line is gone.

**Done** in #525. Each `self` holds a `ClosureSite` of its template's top-level scope, taken
on the context that owns that scope (`RenderContext::FindSelf` walks up from a `with` or block
context, which `self` outlives); a call renders the block on a child of that home linked at the
top level, as `super()` does, and keeps the frame search for the blocks. Pinned by
`loader.importer_block_via_self_in_imported_macro`, `importer_block_via_self_sees_later_set`,
`self_block_ignores_loop_and_with_names` and `self_taken_in_with_called_after`.
