---
status: in-progress
priority: low
area: parity
depends: [0523]
touches: [python/src/, src/internal_value.cpp#DictView, src/internal_value.cpp#ListConverter, src/statements.cpp#ForStatement, src/value_visitors.h#ValueRenderer]
shares: [test/parity/cases/operators.py, test/parity/divergences/operators.txt]
---
# What dict views and dict walks still lack after 0523

**Problem.** 0523 made `d.keys()`, `values()` and `items()` read their dict live, with Python's
`dictionary changed size during iteration` when a walk sees the size change. The walk itself
(`DictViewEnumerator`) takes the view's items when it starts, because the dict's entries live in a
`std::list` whose iterators an erase invalidates and `TemplateDict` has no room for a version
counter (its layout is pinned by a static_assert, docs/tasks/0483). Left (Jinja2 3.1.6):

1. A value changed under an existing key while a walk runs is read as it was when the walk began:
   `{% for k, v in e.items() %}{% do e.update({'b': 9}) %}{{ v }}{% endfor %}` prints `12` for
   `{'a': 1, 'b': 2}`, Python `19` (`operators.dict_view_iter_value_change`).
2. A walk over a dict that loses one key and gains another (same size) reads its starting keys:
   Python gives `ac`, the engine `ab` (`operators.dict_view_iter_replace`). Python raises
   `dictionary keys changed during iteration` in some such walks; both raising would match.
3. A `for` loop over the dict itself (`{% for k in e %}`) walks a copy of its keys
   (`ConvertToList`), so it neither sees the change nor raises (`operators.dict_iter_grow`).
4. A view in a reference cycle prints its inner list's `[...]` where Python prints `...`
   (`{% do e.update({'v': e.values()}) %}{{ e }}`, `operators.dict_view_values_cycle_repr`).
5. `d.keys() | indent` fails with `'dict_keys' object has no attribute 'splitlines'`; Python's
   message is `unsupported operand type(s) for +=: 'dict_keys' and 'str'`. Message only.
6. In the Python module an engine view goes to Python as a list (CreateGenericList gives the
   host a snapshot of the view's kind), so a host function sees a list; and a set (`d.keys() - []`)
   goes as a list too. Ordering of two host views is 0381. A host dict's own views (the module's
   context is Python dicts) have no `-` in the engine, and their `mapping` is Python's mappingproxy
   as a host object, which `pprint` and `tojson` treat as a mapping (module-divergences.txt lines
   `operators.dict_view_set_*`, `operators.dict_view_mapping_proxy_*`).
7. A `set` (0523) has only `isdisjoint`, `-` with another set or view, `in`, `len` and comparison;
   none of Python's other set methods (`union`, `add`, ...). pprint of a long set breaks it over
   lines as a list would, unsorted (Python sorts it).
8. (From 0490's security review.) In the Python module's `SandboxedEnvironment`, `{{ range.mro() }}`
   (also `dict`, `cycler`) renders `[<class 'range'>, <class 'object'>]` where Jinja2's
   `is_internal_attribute` refuses `mro` of a type with SecurityError: `PySandboxPolicy::NativeAttribute`
   (python/src/adapters.cpp) passes every name without a leading underscore, and the policy gets a
   stand-in that is no type. Not exploitable (the stand-in classes reach only `object`, which cannot be
   built), a parity gap of the sandbox.
9. (From the 0523/0490 verifier.) Set edges: a set `in` a view or set raises `unhashable type: 'set'`
   (Python makes it a frozenset and answers False); `view - undefined` fails (Python: the set unchanged);
   `set | last` works (Python: `'set' object is not reversible`); `urlencode`, `xmlattr` and `random` of a
   mappingproxy succeed where Python fails; `pprint` of a mappingproxy in a cycle fails with the recursion
   limit (Python `{'p': mappingproxy({...})}`); `{{ (1).__class__.mro }}` prints empty (Python a bound
   method); `str(1, 2)`, `Markup(x=1)`, `Undefined(x=1)` and `mro(1)` take arguments Python refuses; an
   `int()`/`float()` error quotes the whole text (Python too, but a cap would be kinder to logs).

**Proposal.** 8: the bridge refuses `mro` of a class object as Jinja2 does. 1-2: a walk of a dict the template owns that looks each key up again by its token
when the dict's size is unchanged but an entry was erased; that needs either a modification count
the layout pin allows (a bit in `KeyTableHolder`, or a count kept by the accessor that owns the
`TemplateDict`) or a lookup per item for template-owned dicts only (host maps cannot change). Measure
`for k, v in d.items()` with `bench/count.py` before choosing. 3: the loop iterates a keys
`DictView` of a dict the template owns (statements.cpp, after batch 9 C lands). 4: the view enters
the container stack under its dict's identity. 6: a host view type in the module (python/src).

**Resolution (batch 10 A).** Items 1-4, 8 and 9 are fixed: a walk of a dict the template owns, and a `for` loop
over it, read it live (Python's `changed size` and `keys changed` errors included); a view inside itself prints `...`;
the item 9 edges match Python or turned out to match already (a set `in` a view raises in Python too; `xmlattr` of a
mappingproxy works in Python); `mro` of a class is refused in the module's sandbox. Items 5-7 and the edges found on
the way moved to 0543.

**Done when** items 8 and 9 have corpus cases that match or are recorded deliberate, and the four corpus cases above match in narrow and wide runs and their divergence lines go.
