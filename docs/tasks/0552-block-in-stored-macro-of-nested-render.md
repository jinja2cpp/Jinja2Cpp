---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/967
priority: low
area: parity
depends: [0491]
touches: [src/statements.cpp#BlockStatement]
shares: [test/borrowed_cells_test.cpp]
---
# A block inside a macro a nested render stored renders empty once that render has ended

**Problem.** Found by the verifier of 0531. A render nested in another one that shares its globals stores a macro
whose body holds a block; the outer render calls it after the nested render ended. With the nested template
`{% set x = 'X' %}{% macro m() %}{% block b %}{{ x }}{% endblock %}{% endmacro %}{% set _ = g.append(m) %}` and the
outer `{{ inner() }}{{ g[-1]() }}`, Python Jinja2 3.1.6 prints `X` and Jinja2C++ prints nothing (master too).
Memory-safe. The block's template frame (its blocks stack) ended with the nested render, so the call finds no block.

**Proposal.** Keep the blocks a macro's body renders with the macro's capture when its frames end before the render
(as 0501 keeps `super`), or resolve the block from the macro's own tree when the frame is gone.

**Done when.** A `BorrowedCellsSingle.NestedRenderValueOutlivesItsTemplate` row with the templates above prints `X`,
ASan clean.
