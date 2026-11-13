---
status: open
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
