---
status: open
priority: low
area: parity
touches: [src/value_visitors.h, src/serialize_filters.cpp#PrettyPrinter]
---
# str() and repr() of builtin globals and imported templates

**Problem.** Found while fixing 0215 (`filters.pprint_global_and_module`). A builtin global
prints as nothing with `{{ }}` and as `<callable>` with `pprint`; Python prints its class or
function: `{{ range }}` and `{{ range|pprint }}` are `<class 'range'>`, `{{ dict|pprint }}` is
`<class 'dict'>`. A template imported with `{% import 'lib' as m %}` prints as the dict of its
exports (`{'x': 1}`) with both; Jinja2 prints `{{ m }}` as the module's output (empty here)
and `{{ m|pprint }}` as `<TemplateModule 'lib'>`.

**Proposal.** Give the builtin global callables a Python repr (class for `range`, `dict`,
`namespace`, `cycler`, `joiner`; `<function ...>` for `lipsum` cannot match, it names an
address) and have the visitors print it for str() and repr(). Mark imported module maps with
an object class whose str() is the module's rendered body and repr() is
`<TemplateModule 'name'>` (see 0284 for the module object).

**Done when.** `filters.pprint_global_and_module` matches and its line is gone from
`test/parity/divergences/filters.txt`.
