---
status: open
priority: low
area: parity
depends: []
touches: [src/statements.cpp]
---
# A call block on an undefined name or a non-callable value

**Problem.** `{% call m() %}{% endcall %}` with `m` undefined raises `UndefinedError: 'm'
is undefined` in Jinja2, and with `{% set m = 1 %}` raises `TypeError: 'int' object is not
callable`. Jinja2C++ renders nothing in both cases: `MacroCallStatement::Render` returns
when the name is missing or is not a statement callable. Found by the verifier on the
0118 P5b PRs.

**Proposal.** Report the missing name through the undefined policy (as a call expression
on an undefined name does) and raise the not-callable error otherwise. Check how a call
block on a user callable (`Callable::Type::Expression`) behaves in Jinja2 before changing
that branch.

**Done when** both cases fail at render as in Jinja2, with corpus cases.
