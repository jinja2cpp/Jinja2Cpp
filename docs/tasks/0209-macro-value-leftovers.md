---
status: open
priority: low
area: parity
touches: [src/filters.cpp, src/expression_evaluator.cpp]
---
# Macros as values: what 0208 left over

**Problem.** 0208 made macros compare by identity and print their repr in `pprint`. Other uses
of a macro as a value still differ from Jinja2 3.1.6, each pinned by a corpus case and a
divergences line:
- `m|truncate(4)` renders ''; Jinja2 raises TypeError (object of type 'Macro' has no len())
  (`statements.macro_truncate_rejected`).
- `dict(a=m)|xmlattr` prints nothing; Jinja2 prints the escaped repr, ` a="&lt;Macro &#39;m&#39;&gt;"`
  (`statements.macro_xmlattr`).
- `m.__call__()` is rejected ('function object' has no attribute '__call__'); Jinja2 calls the
  macro (`statements.macro_dunder_call`).

Block references ignoring their arguments (`{{ self.b(1) }}`) belongs with 0198.

**Done when.** No divergences line names 0209 and `ctest -R parity` passes.
