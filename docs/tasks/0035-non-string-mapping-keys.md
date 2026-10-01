---
status: open
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
collide, and an unhashable key such as `[1]` is accepted instead of raising
`TypeError` (2 cases: `literals.dict_int_key`, `errors.dict_unhashable_key`).

**Proposal.** Cheap step: look up an integer subscript on a mapping by its decimal
spelling, and make `DictCreator` reject list and mapping keys with a render error.
Full fix: a key type in the value model (a variant of string, integer, bool, None,
tuple), which touches the public API and is worth doing only together with 0031.

**Done when.** No line of `test/parity/divergences.txt` names task 0035, and `ctest -R parity` passes.
