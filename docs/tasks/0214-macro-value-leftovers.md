---
status: in-progress
priority: low
area: parity
touches: [src/filters.cpp, src/expression_evaluator.cpp, src/statements.cpp]
---
# Macros and methods as values: what 0208 and 0145 left over

**Problem.** 0208 made macros compare by identity and print their repr in `pprint`. Other uses
of a macro as a value still differ from Jinja2 3.1.6, each pinned by a corpus case and a
divergences line:
- `m|truncate(4)` renders ''; Jinja2 raises TypeError (object of type 'Macro' has no len())
  (`statements.macro_truncate_rejected`).
- `dict(a=m)|xmlattr` prints nothing; Jinja2 prints the escaped repr, ` a="&lt;Macro &#39;m&#39;&gt;"`
  (`statements.macro_xmlattr`).
- `m.__call__()` is rejected ('function object' has no attribute '__call__'); Jinja2 calls the
  macro (`statements.macro_dunder_call`).
- `self.b == self.b` is True: the template's `self` stores one callable per block, which now
  compares by identity; Jinja2 makes a new BlockReference on each read, so it is False
  (`statements.block_reference_new_each_read`). Before 0145 it was False by accident.
- `loop.cycle == loop.cycle` is False: each read makes a new callable; Python's bound methods
  of one object compare equal (`statements.loop_method_equals_itself`). Methods of reflected
  structs likely behave the same way.

Block references ignoring their arguments (`{{ self.b(1) }}`) belongs with 0198.

**Decision (2026-10-24, 0220 PR).** `self.b == self.b` stays True, as a deliberate divergence:
matching Jinja2 means allocating a new callable on every read of `self.b`, `self.b()` calls
included, for an identity check no template relies on. Its divergences line says
`deliberate`. The other four are fixed: `xmlattr` prints a macro's repr, `f.__call__` is `f`,
and `loop.cycle`/`loop.changed` compare equal across reads of one loop
(`Callable::SetBoundMethod`); the corpus pins them in `calls.*`. `f.__call__` is `f` itself rather
than a bound method, so `m.__call__ == m` is True and it prints as `m` does; also deliberate
(`calls.dunder_call_is_a_bound_method`).

**Done when.** The only divergences lines naming 0214 are the deliberate ones and `ctest -R parity`
passes.
