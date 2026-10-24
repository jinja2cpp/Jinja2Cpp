---
status: open
priority: low
area: parity
depends: [0305]
touches: [src/serialize_filters.cpp#PrettyPrinter]
---
# `pprint` of builtin globals and imported templates

**Problem.** Found while fixing 0215 (`filters.pprint_global_and_module`). A template imported
with `{% import 'lib' as m %}` prints as the dict of its exports (`{'x': 1}`) with both `{{ m }}`
and `pprint`; Jinja2 prints `{{ m }}` as the module's output (empty here) and `{{ m|pprint }}`
as `<TemplateModule 'lib'>`. A builtin global prints `<callable>` with `pprint` (Python:
`<class 'range'>`, `<class 'dict'>`); 0305 gives the globals their class text for `{{ }}`,
and `PrettyPrinter` has to print the same.

**Proposal.** After 0305, have `PrettyPrinter`'s callable overload print the global's repr
text. Mark imported module maps with an object class whose str() is the module's rendered body
and repr() is `<TemplateModule 'name'>` (see 0284 for the module object).

**Done when.** `filters.pprint_global_and_module` matches and its line is gone from
`test/parity/divergences/filters.txt`.
