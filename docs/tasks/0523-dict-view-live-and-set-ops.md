---
status: open
priority: low
area: parity
depends: [0496]
touches: [src/value_methods.cpp#DictKeys, src/internal_value.cpp#CreateValuesAdapter, src/value_visitors.h#SetViewOperands]
shares: [test/parity/cases/operators.py, test/parity/divergences/operators.txt]
---
# What dict views still lack after 0496

**Problem.** 0496 gave the views of `d.keys()`, `values()` and `items()` their dict (`mapping`), the
reverse iterators of their own kinds and Python's hashing `in`. Left (Jinja2 3.1.6):

1. A view is a snapshot of its items, not a live view: `{% set k = d.keys() %}{% do d.update({'z': 1}) %}{{ k }}`
   misses `z`, and a dict changed while a loop walks its view raises `dictionary changed size during
   iteration` in Python but not here. `in` of a keys or items view already asks the live dict (0496), so the
   snapshot and `in` can disagree after a change; Python's never do.
2. `mapping` is the dict itself, where Python's is a read-only `mappingproxy`: `d.keys().mapping.__class__`
   prints `dict`, and `d.keys().mapping.update(...)` changes the dict (Python: no attribute `update`).
3. Keys and items views have no `-`, `|`, `&`, `^` (Jinja2 gives sets), and `isdisjoint` and set comparison
   build the lookup per call (linear, but rebuilt in a loop).
4. A view in a reference cycle prints the inner list's `[...]` where Python prints `...`.
5. `d.keys() | indent` fails with `'dict_keys' object has no attribute 'splitlines'`; Python's message is
   `unsupported operand type(s) for +=: 'dict_keys' and 'str'`. Message only, which the corpus does not compare.
6. In the Python module an engine view goes to Python as a list, so a host function that gets one sees a
   list (0496 compares a host dict's `dict_keys` with the engine's view as sets in the engine, and the host's
   reversed() of a built-in type gets its iterator kind). Ordering of two host views is 0381.

**Proposal.** Make the view read its dict's entries live (the view keeps the dict since 0496), with the size
check Python's iteration does; a read-only map adapter over the dict for `mapping`; the set operators of
Jinja2's sets once sets exist (they print as `{...}`).

**Done when** cases for 1-3 match in narrow and wide runs: `operators.dict_view_mapping_proxy` (its
divergence line goes) and new cases for live views and the set operators.
