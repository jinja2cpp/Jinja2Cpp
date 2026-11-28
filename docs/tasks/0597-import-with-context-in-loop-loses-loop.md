---
status: open
priority: low
area: parity
depends: []
touches: [src/statements.cpp#ImportStatement]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt]
---
# A module imported with context in a loop does not see the loop's `loop`

**Problem.** Found by the verifier of batch 14 C (0584), identical on master c4561b9.
`{% for i in [1] %}{{ loop.index }}{% import "a" as m with context %}{{ m.f() }}{% endfor %}` with `a` =
`{% macro f() %}A{{ i }}{{ loop is defined }}{% endmacro %}` prints `1A1False`; Jinja2 3.1.6 prints `1A1True`, as its
module context is made of `context.get_all()` and the frame's locals, `loop` among them when the body names it.
`{% from ... import ... with context %}` and an import in a scoped block in a loop behave the same. `include` passes
the loop by the rule of HideIncludingLoop; an import with context passes none. Pinned by the corpus case
`statements.import_with_context_in_loop_sees_loop`.

**Proposal.** Give an import with context the `loop` an include would see (HideIncludingLoop's rule), checked against
Jinja2 for a body that does not name `loop` and for a module cached across passes.

**Done when.** The corpus case matches and its divergence line is gone, narrow and wide.

**Progress (batch 15 A).** ImporterNames adds the `loop` LoopSeenWithContextOf finds (HideIncludingLoop's rule). 8
corpus cases: from-import, a body that does not name `loop`, an inner loop, a scoped block, a macro stored past the loop
(the module's copy of `loop` keeps the frame out of the pool), a top-level `set loop`, the module's top level.
