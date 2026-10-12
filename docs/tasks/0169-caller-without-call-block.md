---
status: open
priority: low
area: parity
depends: [0165]
touches: [src/statements.cpp#MacroCallStatement]
shares: [test/parity/cases/errors.py]
---
# `caller()` without a call block raises

**Problem.** In Jinja2 a macro whose body reads `caller` gets a `caller` that raises
`UndefinedError: No caller defined` when it was not called from a call block, and a call
body that reads `caller` binds its own, so it does not reach the enclosing macro's.
Jinja2C++ renders nothing in both cases (`errors.caller_without_call_block`,
`errors.call_body_reads_own_missing_caller`).

**Proposal.** Bind `caller` in every macro whose body reads it (the resolver already knows),
to a callable that raises "No caller defined" unless a call block supplies one; do the same
for a call body that reads `caller`. After 0165 the call body runs at its definition site,
which may change the second case's output: re-check its kind when 0165 lands.

**Done when.** No divergences line names 0169 and `ctest -R parity` passes.
