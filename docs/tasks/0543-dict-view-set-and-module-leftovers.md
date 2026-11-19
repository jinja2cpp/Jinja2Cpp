---
status: open
priority: low
area: parity
depends: [0538]
touches: [python/src/, src/value_methods.cpp#SetMethods, src/filters.cpp#Indent]
shares: [test/parity/cases/operators.py, test/parity/divergences/operators.txt, python/tests/parity/module-divergences.txt]
---
# Sets, module host views and the indent message after 0538

**Problem.** 0538 made a dict's walks live and closed the edges of the engine's sets and mappingproxies the corpus
pinned. Three items of it stayed out of that PR, because each is its own piece of work and none of them blocks a
template that works in Jinja2 from working here (Jinja2 3.1.6):

1. `d.keys() | indent` fails with `'dict_keys' object has no attribute 'splitlines'`; Python's message is
   `unsupported operand type(s) for +=: 'dict_keys' and 'str'`. Both fail; message only.
2. In the Python module an engine view goes to Python as a list (CreateGenericList gives the host a snapshot of the
   view's kind), so a host function sees a list; a set (`d.keys() - []`) goes as a list too. A host dict's own views
   (the module's context is Python dicts) have no `-` in the engine, and their `mapping` is Python's mappingproxy as a
   host object, which `pprint` and `tojson` treat as a mapping (python/tests/parity/module-divergences.txt lines
   `operators.dict_view_set_*`, `operators.dict_view_mapping_proxy_*`). Ordering of two host views is 0381.
3. A `set` (0523) has only `isdisjoint`, `-` with another set or view, `in`, `len` and comparison; none of Python's
   other set methods (`union`, `intersection`, `add`, `|`, `&`, `^`, ...). `pprint` of a long set breaks it over lines
   as a list would, unsorted (Python sorts it).

**Proposal.** 1: the indent filter reports a non-string the way Python's `+=` does. 2: a host view type in the module
(python/src) that hands Python a real view or set of the engine's items, and the engine's set operators for a host
view, read through its items. 3: the set methods and operators over the engine's set (value_methods.cpp), with corpus
cases for each.

**Done when** the module-divergences lines above are gone, the set methods have corpus cases that match in narrow and
wide runs, and `d.keys() | indent` reports Python's message.
