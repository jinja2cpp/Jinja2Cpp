---
status: open
priority: low
area: robustness
depends: []
touches: [src/statements.cpp#ImportStatement, src/statements.cpp#IncludeStatement, src/statements.cpp#BlockStatement::RenderBody]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt]
---
# An import in a loop keeps ~5 KB per pass, and two small `super`/include gaps

**Problem.** Found by the verifier of batch 15 A, the same on master b3313a2 (not from that diff):
1. `{% for x in range(n) %}{% import "m" as m with context %}{% endfor %}` keeps about 5.1 KB per pass until the render
   ends, with nothing stored (2,000 to 20,000 passes: 21.8 MB to 114 MB). Each import with context adds a module the
   render owns (IRendererCallback::AddModule); a module no value reaches could go at the end of the pass. This is a
   memory-budget concern for untrusted templates (the budget counts it, but the growth is avoidable).
2. `{% set ns.f = super %}` in a block with no parent block, then `{% if ns.f %}`: the engine fails, Jinja2 3.1.6 treats
   the missing parent's reference as undefined.
3. An include that names `loop` where no `loop` is visible: Jinja2 raises UndefinedError, the engine renders empty.

**Done when.** (1) a loop of imports with context that stores nothing keeps a flat memory use (a counted-allocation
test); (2) and (3) have corpus cases that match Jinja2, narrow and wide.
