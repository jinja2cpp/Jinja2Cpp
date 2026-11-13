---
status: open
priority: low
area: parity
depends: [0489]
touches: [src/value_methods.cpp#DictViewMethods, src/testers.cpp#IsValueIn, src/filters.cpp#Reverse, python/src/adapters.cpp]
shares: [test/parity/cases/operators.py, test/parity/divergences/operators.txt]
---
# What dict views still lack after 0489

**Problem.** 0489 made `d.keys()`, `values()` and `items()` views (Jinja2 3.1.6). Left:

1. `d.keys().mapping` is undefined; Python gives a `mappingproxy` of the dict, which prints as
   the dict (`operators.dict_view_mapping`). The view keeps its dict only when the dict needs a
   lifetime anchor (`SetParentData`), so the view needs a reference to it.
2. `reversed()` of `dict_items` and `dict_values` is a `list_reverseiterator`; Python's are
   `dict_reverseitemiterator` and `dict_reversevalueiterator` (`operators.dict_view_reversed_class`).
   `IteratorKind` has one free value in its four bits, so two more kinds need a bit or a merge.
3. `[1] in d.keys()` and `([1], 2) in d.items()` are `False`; Python hashes the item and raises
   `unhashable type: 'list'` (`operators.dict_view_in_unhashable`). `in` walks the view
   (`testers::IsValueIn`); `DictViewLookup` (value_methods.cpp) does Python's lookup.
4. `d.keys() | indent` fails with `'dict_keys' object has no attribute 'splitlines'`; Python's
   message is `unsupported operand type(s) for +=: 'dict_keys' and 'str'` (a list extends by the
   string first). Message only, which the corpus does not compare.
5. A view is a snapshot of its dict, not a live view: `{% set k = d.keys() %}{% do d.update({'z': 1}) %}{{ k }}`
   misses `z`, and a dict changed while a loop walks its view raises `dictionary changed size
   during iteration` in Python but not here. A view in a reference cycle prints the inner list's
   `[...]` where Python prints `...`. `isdisjoint` and set comparison build the lookup per call
   (linear, but rebuilt in a loop), and keys views have no `-`, `|`, `&`, `^` (Jinja2 sets).
6. In the Python module a host dict's `keys()` is Python's own `dict_keys`, while a literal's is
   the engine's view, which goes to Python as a list: `d.keys() == {'a': 0}.keys()` is `False`
   (`operators.dict_view_eq_set` in python/tests/parity/module-divergences.txt), and `random` of a
   host `dict_keys` picks an item where Python fails (`operators.dict_view_random`). Ordering of
   two host views is 0381 (`operators.dict_view_subset`).

**Proposal.** Keep the source dict in the view (a parent value set always, as the lifetime anchor
is), add `mapping` as a view attribute; find room for the two reverse kinds; let `in` of a
set-like view hash the item.

**Done when** the three `operators` divergence lines above are gone, and the module's 0492 lines
in python/tests/parity/module-divergences.txt.
