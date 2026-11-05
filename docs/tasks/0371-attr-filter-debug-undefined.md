---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/774
priority: low
area: parity
depends: []
touches: [src/filters.cpp#Attribute, src/undefined.cpp, src/undefined.h, test/parity/cases/filters.py, test/parity/cases/undefined.py]
---
# `|attr` of a missing attribute under DebugUndefined prints empty

**Problem.** Found by the 0297/0307 verifier. `{{ 'x'|attr('f') }}` under DebugUndefined renders
`{{ no such element: str object['f'] }}` in Jinja2 3.1.6 and an empty string in Jinja2C++: the filter's
undefined result carries no name or object, so DebugUndefined has nothing to print. Not in the
corpus yet: `filters.py` is group A's area in parity batch 2, so the case is added with the fix.

**Proposal.** Build the result with `MakeUndefined(context, obj, key)`, as subscripts do.

**Done when** a `filters.attr_missing_debug` case (narrow and wide) matches Python.

**Resolution.** `|attr` builds its missing result as `environment.undefined(obj=obj, name=name)`
(`MakeUndefinedAttr`, which names the attribute even when a plain dict has an item of that name),
and an undefined attribute names an engine object's type with its module, as Jinja2's
`object_type_repr` does (`jinja2.utils.Namespace object`, `jinja2.runtime.LoopContext object`,
`jinja2.runtime.Macro object`). Cases `filters.attr_missing_debug`, `filters.attr_missing_strict`,
`filters.attr_missing_strict_dict_item`, `undefined.debug_engine_object_types`,
`undefined.debug_callable_object_types`, `undefined.strict_engine_object_type`. Parity batch 4
group A.
