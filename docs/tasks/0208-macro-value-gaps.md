---
status: open
priority: low
area: parity
touches: [src/value_visitors.h, src/filters.cpp]
shares: [test/parity/cases/statements.py]
---
# Macros as values

Found by the 0199 verifier. 0199 gave macros their repr (`<Macro 'm'>`), but other uses
of a macro as a value still differ from Jinja2 3.1.6:

- `m|pprint` prints `<callable>`; Jinja2 prints `<Macro 'm'>` (`statements.macro_pprint`).
- `m == m` is False and `[m, m]|unique` keeps both; Jinja2 compares macros by identity
  (`statements.macro_equals_itself`).
- `m|truncate(4)` renders ''; Jinja2 raises TypeError. `dict(a=m)|xmlattr` prints nothing.
- `m.__call__()` is rejected.
- Call blocks pass `caller` to callees that do not take it: `{% call dict() %}` prints
  `{'caller': <Macro anonymous>}`, and `{% call self.b() %}` / `{% call super() %}` render,
  where Jinja2 raises TypeError (unexpected keyword argument 'caller'). Block references
  ignore their arguments in general (`{{ self.b(1) }}`), which belongs with 0198.

**Done when.** No divergences line names 0208 and `ctest -R parity` passes.
