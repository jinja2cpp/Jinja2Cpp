---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/721
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

**Done.** `pprint` prints a global's class through 0305's `Callable::SetRepr`. `import ... as m`
binds a `ModuleMapAdapter` (internal_value.cpp, `CreateModuleAdapter`, made by
`ImportedModule::MakeObject`): an object of class `TemplateModule` whose repr argument is the
template's name and whose `IMapAccessor::GetStrValue` is the output of the module's top level,
which `ImportedModule::Render` now keeps. `{{ m }}` prints that output, unescaped under
autoescape as Jinja2's `__html__` gives it (markup.h); `pprint`, `[m]` and `string` give
`<TemplateModule 'lib'>`, and being an object, `m|length`, `for k in m`, `m|list`, `m.items()`,
`m|dictsort` and `m|tojson` fail as in Jinja2 (loader `module_object_*` cases). `m is escaped`
stays False: 0367.
