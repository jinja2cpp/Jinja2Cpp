---
status: open
priority: medium
area: parity
depends: [0001]
touches: [src/statements.cpp, src/statements.h, src/template_parser.cpp, src/template_impl.h, test/parity/]
---
# Template inheritance and import semantics

**Problem.** Inheritance and imports mostly work, but (12 cases): `super()` skips a level
in three-level inheritance; `extends` with a variable renders nothing and inside `if`
does not parse; unscoped blocks see loop variables (Jinja2 hides them unless `scoped`);
`required` blocks and `self.blockname()` are unsupported; inside a macro, `name`
resolves to the macro's own name, which also hides the import-context rules; importing
`_private` names is allowed; `endblock b` after `block a`, duplicate blocks and double
`extends` are accepted.

**Proposal.** Fix the `super()` chain and the macro-scope leak first (wrong output, no
error); then dynamic and conditional `extends`, block scoping, `required`, `self`, and
the structural errors.

**Done when.** No line of `test/parity/divergences.txt` names task 0023, and `ctest -R parity` passes.
