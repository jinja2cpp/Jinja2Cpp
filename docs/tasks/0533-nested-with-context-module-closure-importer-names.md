---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/954
priority: low
area: parity
depends: [0491]
touches: [src/template_impl.h, src/statements.cpp]
shares: [test/borrowed_cells_test.cpp]
---
# A macro of a module imported with context by a nested render loses the importer's names once that render ends

**Problem.** Found by the verifier of 0525. A render nested in another one that shares its globals
imports a module with context and stores one of its macros in a shared global. Once the nested
render has ended, the outer render calls the macro: the names the module read from its importer
are gone. With `m.j2` = `{% set t = x %}{% macro tm() %}[{{ t }}{{ x }}]{% endmacro %}`, the nested
template `{% set x = 'cl' %}{% import 'm.j2' as m with context %}{% set _ = g.append(m.tm) %}` and the
outer `{{ inner() }}{{ g[-1]() }}`, Jinja2C++ prints `[cl]` where Python Jinja2 3.1.6 prints `[clcl]`.
The same whether the nested render succeeds or fails. Memory-safe.

**Proposal.** When the nested render closes over its modules (`RendererCallback::CaptureClosuresForOuter`),
a module imported with context keeps the names it sees below its own scopes, as the importer's
capture does (0491), rather than only its top level.

**Done when.** A `BorrowedCellsSingle.NestedRenderValueOutlivesItsTemplate` row with the templates
above prints `[clcl]`, ASan clean.
