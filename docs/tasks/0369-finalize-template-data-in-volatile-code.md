---
status: open
priority: low
area: parity
depends: []
touches: [src/renderer.h, src/template_parser.cpp, test/parity/divergences/autoescape.txt]
---
# Template text in volatile autoescape code goes through finalize

**Problem.** Found by the 0297/0307 verifier. With a `finalize` callable, Jinja2 3.1.6 finalizes the
template text inside an `{% autoescape <expr> %}` block whose value is known only at run time
(volatile code): `{% autoescape on %}a{{ 1 }}{% endautoescape %}` with `on=True` and
`finalize=lambda v: f"[{v}]"` renders `[a][1]` in Python and `a[1]` in Jinja2C++. Outside volatile
code (`{% autoescape true %}`) both give `a[1]`. Python's `CodeGenerator.visit_Output` cannot fold
the text there (`_output_child_to_const` raises Impossible for a volatile eval context), so the text
takes the run-time path that calls finalize. Case: `autoescape.volatile_finalize_template_data`.

**Proposal.** In volatile code, render template data through the finalize callable as the
run-time path of `FinalizedExpressionRenderer` does, escaping by the runtime setting.

**Done when** the case's divergence line is deleted and `ctest -R parity` passes.
