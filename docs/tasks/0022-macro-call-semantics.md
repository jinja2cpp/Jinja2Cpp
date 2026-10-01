---
status: in-progress
priority: medium
area: parity
depends: [0001]
touches: []
shares: [src/statements.cpp, src/statements.h, src/template_parser.cpp]
---
# Macro call semantics

**Problem.** Macro calls are not validated: extra positional arguments and unknown
keywords are silently accepted (Jinja2 raises `TypeError` unless the macro uses
`varargs`/`kwargs`); invalid signatures (`m(a, a)`, `m(a=1, b)`) load; a default cannot
refer to an earlier argument (`m(a, b=a)`); `macro.name` and `macro.arguments` are
missing (6 cases).

**Proposal.** Check signatures at parse time, check calls at render time with
Jinja2's error messages, evaluate defaults in a scope that sees earlier arguments, and
expose the macro object's attributes.

**Done when.** No line of `test/parity/divergences.txt` names task 0022, and `ctest -R parity` passes.
