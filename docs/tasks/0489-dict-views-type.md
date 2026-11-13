---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/879
priority: low
area: parity
depends: [0475]
touches: [src/value_methods.cpp#DictItems, src/value_visitors.h#ValueRenderer]
shares: [test/parity/cases/operators.py, test/parity/divergences/operators.txt]
---
# `d.items()`, `d.keys()` and `d.values()` are lists, not dict views

**Problem.** Python's dict views have types of their own; the engine returns lists. Found by 0475
(Jinja2 3.1.6):

- `{{ d.items() }}` prints `dict_items([('a', 1)])` in Python, `[('a', 1)]` here; `d.keys()` and
  `d.values()` likewise (`dict_keys([...])`, `dict_values([...])`).
- `{{ {'a': 1}.items() * 2 }}` raises `unsupported operand type(s) for *: 'dict_items' and 'int'`
  in Python and renders a list of four pairs here (`operators.dict_items_times_int`, accepts).
- `{{ d.keys() == ['a'] }}` is `False` in Python (a view equals no list), `True` here.
- Messages name `list` where Python names the view: `d.items() * 'a'` says `'str'` where Python
  says `can't multiply sequence by non-int of type 'dict_items'`; `d.items().__class__` is
  `<class 'list'>` where Python's is `<class 'dict_items'>` (0476).

**Proposal.** Mark the list a dict method returns with its view kind (as a range or a generator is
marked, `IteratorKind`): repr `dict_items([...])`, `PythonTypeName` the view's name, no `*` or
`+`, equality only with a view of the same kind (`dict_keys` and `dict_items` compare as sets in
Python: decide whether to follow or record it deliberate). Iteration, `length`, `in`, filters and
`list` stay as now. Keys views support `&`, `|`, `-`, `^` in Python, which Jinja2 has no operators
for, so nothing is lost there.

**Done when** `operators.dict_items_times_int` matches and its divergence line is gone, with cases
for the repr and the comparison.

**Resolution.** `IteratorKind` gained `DictKeys`, `DictValues` and `DictItems`, which
`d.keys()`, `values()` and `items()` set on the list they return. A view kind is no iterator
(`IteratorTypeName` stays null) but takes the iterator paths that fit Python's views: no items,
slices, list methods, `+` or `*`, no `is sequence`, unhashable as a dict key. It keeps `len()`
(`length`, `truncate`) and `reversed()` (`dict_keys` reverses into a `dict_reversekeyiterator`,
values and items into a `list_reverseiterator`, see 0496). `DictViewTypeName` names it for repr
(`dict_keys([...])`), `PythonTypeName`, `%`/format, `tojson`'s error, `random`'s "not
subscriptable" and `__class__` (`dict_keys`, `dict_values`, `dict_items` joined the class table).

Comparison follows Python, not a deliberate divergence: two set-like views (keys, items, of
either kind) compare as sets, by length and one token lookup per item (`AllInDictView`, linear,
not quadratic); `dict_values` and a view against a list equal only by identity; ordering is a
set comparison between set-like views and a TypeError otherwise. `isdisjoint` is the one view
method (keys and items). `list.extend` now takes any iterable (a string, a dict), which the same
iterable conversion `isdisjoint` needed showed missing.

Cases: `operators.dict_view*`, `operators.list_plus_dict_view`, `operators.dict_values_*`,
`methods.list_extend_iterables`, `methods.list_extend_int`. Leftovers: 0496.
