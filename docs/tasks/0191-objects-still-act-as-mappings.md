---
status: open
priority: low
area: parity
depends: [0042]
touches: [src/serialize_filters.cpp, src/statements.cpp#AssignTo, src/value_visitors.h#BinaryMathOperation, src/global_functions.cpp, src/filters.cpp#Attribute]
---
# namespace, cycler and joiner still act as mappings in a few places

**Problem.** 0042 made `namespace()`, `cycler()` and `joiner()` objects
(`IMapAccessor::GetObjectClass()`): not mappings, not iterable, printed as Python's repr.
The 0042 verifier found places that still treat them as the maps they are stored as:

- `ns|tojson` prints `{"a": 1}`; Python raises "Object of type Namespace is not JSON
  serializable" (`globals.namespace_tojson`). `ns|items`, `ns|dictsort`, `ns|xmlattr` work
  where Python raises; `ns|pprint` prints `{'a': 1}` and `cycler(1)|pprint` prints its
  internals, where Python prints the repr.
- `{% set a, b = ns %}` unpacks by name (the "taken by name" branch of `AssignTo`); Python
  raises "cannot unpack non-iterable Namespace object" (`statements.namespace_unpack`).
- `c == c` is false for a cycler and `namespace(a=1) == namespace(a=1)` is true: objects
  compare by identity in Python. An empty namespace is falsy; Python objects are truthy
  (`globals.objects_equality`).
- `joiner('-').sep` is undefined (Python `-`) and `ns|attr('a')` renders empty (Python `1`)
  (`globals.objects_attributes`).
- A non-recursive `loop` is callable in Python (it raises when called) but not here
  (`statements.loop_callable`). `loop is mapping` is true here, false in Python.

- Sequence filters over an object: `cycler(1)|length` renders where Python raises
  TypeError (`globals.cycler_length`). This was 0177's case, but 0177 closed without it, so
  it moved here on 2026-10-16.

**Approach.** Check `GetObjectClass()` at each site: tojson/pprint/items/dictsort/xmlattr,
`AssignTo`, `BinaryMathOperation` on two maps (identity for objects), `ConvertToBool`.
Store `sep` on the joiner; make `attr` read map attributes of objects.

**Done when.** No line of `test/parity/divergences/` names 0191.
