---
status: done
priority: low
area: parity
depends: []
touches: [src/lexer.h, src/name_resolver.cpp]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt]
---
# Names a template sets are not NFKC-normalised

**Problem.** Jinja2 compiles a template to Python source, and Python's parser NFKC-normalises
identifiers, so two names that are NFKC-equal are one variable when the template itself sets
them: `{% set ﬁ = 1 %}{{ fi }}` renders `1`, `{% set 𝑥 = 2 %}{{ x }}` renders `2`. A name read
from the context is looked up by its own spelling (`{{ ﬁ }}` with `fi` in the context renders
empty). Jinja2C++ keeps every spelling a name of its own (`statements.name_nfkc_alias`;
`name_nfkc_context` matches). Found by the 0431 verifier.

**Proposal.** Decide whether to follow it: an artifact of Python code generation rather than a
language rule, and it would mean NFKC tables in the engine for names the template binds (set,
for targets, macro parameters, imports) and their reads, but not context lookups. Either
implement it or record the line as deliberate with the decision.

**Done when.** `statements.name_nfkc_alias` matches or its line says `deliberate` with the
decision.

**Decision (Ruslan, 2026-11-12).** Deliberate: the line stays as `deliberate`. Only templates whose own
names use compatibility characters (ligatures, math or fullwidth letters) differ, and they read the
other spelling as undefined rather than a wrong value; context lookups, ASCII names and ordinary
non-Latin scripts are unaffected. NFKC tables can be added later if a user asks for it.
