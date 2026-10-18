---
status: done
priority: low
area: parity
depends: [0013]
touches: [src/expression_evaluator.cpp#DictCreator]
shares: [src/internal_value.cpp, src/internal_value.h]
---
# Mapping keys that are not strings

**Problem.** Python dicts take any hashable key, and templates occasionally write
`{1: 'x'}` or `{(a, b): c}`; Jinja2C++ mappings are keyed by `std::string` all the way
from `InternalValueMap` to the public `ValuesMap`. Since 0013 a dict literal accepts any
key expression and stores integers and booleans by their spelling (`1` → `'1'`,
`True` → `'True'`), so `{1: 'x'}['1']` works but `{1: 'x'}[1]` does not, `1` and `'1'`
collide, keys of any other type (`1.5`, `none`, `[1]`) all become the empty string, and an
unhashable key such as `[1]` is accepted instead of raising `TypeError` (2 cases: `literals.dict_int_key`, `errors.dict_unhashable_key`).

**Proposal.** Cheap step: look up an integer subscript on a mapping by its decimal
spelling, and make `DictCreator` reject list and mapping keys with a render error.
Full fix: a key type in the value model (a variant of string, integer, bool, None,
tuple), which touches the public API and is worth doing only together with 0031.

**Done when.** No line of `test/parity/divergences/` names task 0036, and `ctest -R parity` passes.

**Shipped (D13 = b).** Dict literals, `dict()`, `namespace()` and the results of the dict
methods take any hashable key (`src/template_dict.*`): `1`, `1.0` and `True` are one key
(first spelling and position kept, last value wins), so are `(1, 2.0)` and `(1.0, 2)`;
`none`, floats, tuples and ranges are keys; a list or a dict raises `TypeError: unhashable
type`. Subscript, `in`, `get`, `setdefault`, `update`, `pop`, `popitem`, `copy`, iteration,
`items`, `dictsort`, `pprint`, `tojson`, `xmlattr`, `urlencode` and `==` follow Python. The
public `ValuesMap`/`GenericMap` keys stay `std::string`.

*Conversion rule at the public boundary* (`CreateGenericMap` of a template dict, what a user
callable receives): a dict with a key that is not a string is handed over as a snapshot, not a
live view, keyed by `str(key)` (`{1: 'x', None: 'y'}` has the keys `"1"` and `"None"`). When two
keys spell alike (`{1: 'x', '1': 'y'}`) the later value wins at the position of the first, as
Python's `{str(k): v for k, v in d.items()}` has it. A dict of string keys stays a live view.
Changing this rule later is a break for callers who rely on it.

*Not done:* object keys (`{namespace(): 1}`, host objects, callables, undefined) have no
identity token yet and all read as the empty string, as before; that is the first bullet of
0212 (`globals.object_dict_key` stays listed). All NaN keys are one key (Python compares NaN by
identity).
