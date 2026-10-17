---
status: done
priority: low
area: parity
depends: [0191]
touches: [src/statements.cpp#AssignTo, src/filters.cpp, src/global_functions.cpp]
shares: [test/parity/divergences/statements.txt, test/parity/divergences/globals.txt]
---
# Objects and dicts still read as maps of names in a few places

**Problem.** 0191 made namespace(), cycler() and joiner() objects at tojson, pprint, items,
dictsort, xmlattr, unpacking, sequence filters, ==, truth, `dict.update` and `%`. Its
verifier found more sites that still read them as maps, all as on master before 0191:

- `cycler(1, 2)|truncate` and `namespace(a=1)|truncate` render empty; Python raises "has
  no len()" (`globals.cycler_truncate`).
- `namespace(a=1)|urlencode` renders `a=1`; Python quotes the repr
  (`globals.namespace_urlencode`).
- `lipsum(namespace())` renders empty, `{{ {namespace(): 1} }}` renders `{'': 1}`; Python
  raises TypeError for both.
- `ns|attr('a', 'b')` renders `1` and `ns|attr(1)` renders empty; `joiner('a')('x')` and
  `joiner('a')(sep='x')` render empty. Python raises for each (argument checks).
- `{% set a, b = {'x': 1, 'y': 2} %}` takes the items named `a` and `b` (the "taken by
  name" branch of `AssignTo`), so both are empty; Python unpacks the keys, `xy`
  (`statements.set_unpack_dict_keys`). A reflected struct is unpacked by name the same
  way; decide whether that host-object extension stays.
- An object that goes through a user callable comes back as a plain map: it loses its
  class and identity (`ident(c) == c` is false, `ident(namespace(a=1))|tojson` is
  `{"a": 1}`), because Value has no object kind.

**Done when.** No line of `test/parity/divergences/` names 0197.

**Resolution.** PR (link below). truncate and urlencode treat an object as Python does;
`set a, b = dict` unpacks the keys, while a host object (a reflected struct or another
user-provided map, which Python could not unpack) keeps the by-name extension; lipsum
checks that its arguments are integers; attr takes one positional argument, a string
(`default=` stays a keyword-only Jinja2C++ extension); joiner's call and cycler's methods
reject arguments. Object dict keys and objects through user callables moved to 0212.
