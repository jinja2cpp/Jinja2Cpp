---
status: done
priority: medium
area: parity
depends: [0165]
touches: [src/statements.cpp#macro]
shares: [src/statements.h, test/parity/cases/statements.py, test/macro_test.cpp]
---
# Macro defaults evaluated per call

**Problem.** Since 0022 a default that names no argument is evaluated once at definition,
so it misses a global reassigned after the definition
(`statements.macro_default_reassigned_global`), and a mutable default is shared between
calls. Python evaluates every default per call, where the macro is defined. 0022 kept the
snapshot only because bodies were dynamically scoped.

**Proposal.** Phase PR 2 of docs/design/0038-lexical-scoping-plan.md (section 4.3):
evaluate every default per call in the body scope after the arguments are bound, through
0165's home call. `defaultRefersToArgs` stays as the flag that binds missing arguments as
undefined first; `MacroDefault`, `definedDefaults` and the mutable-default copy go.

**Done when.** No divergences line names 0166; the rows of plan section 7 (PR 2) are added;
`macro_test.cpp` checks a fresh mutable default per call and a default calling a macro once
per call; `ctest -R parity` passes; perf verdict as in 0165.
