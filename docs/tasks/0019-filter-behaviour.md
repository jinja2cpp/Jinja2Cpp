---
status: done
priority: medium
area: parity
depends: [0001, 0012, 0013, 0034]
touches: [src/serialize_filters.cpp]
shares: [src/filters.cpp, src/filters.h, src/string_converter_filter.cpp]
---
# Filter behaviour divergences

**Problem.** Filters that exist behave differently from Jinja2 (32 cases):
`attr` falls back to item lookup; `batch` pads without `fill_with`; `center` puts the odd
space on the other side; `default` replaces `None` (Jinja2 only replaces undefined);
`dictsort` yields nothing; `int`/`float` do not fall back to `0` and `'3.9'|int` fails;
`format` ignores `%`-placeholders; `groupby` keeps first-seen order instead of sorting,
ignores `default` and its groups do not unpack as `(grouper, list)`; `round` returns an
int and rounds half away from zero, and `method='ceil'`/`'floor'` round negatives
the wrong way (`-10.5|round(method='ceil')` gives `-11.0`, Jinja2 `-10.0`; the `Round`
rows in `test/filters_test.cpp` encode the wrong values); `slice` chunks like `batch`; `sort` ignores
`attribute='a,b'`; `striptags` keeps newlines; `title` keeps inner capitals; `tojson`
uses compact separators; `trim` ignores `chars`; `truncate` measures length differently
and has no `leeway`; `urlencode` quotes like `quote_plus`; `join` rejects `d=`.

**Also (found in 0014).** Filters render a bool argument as empty: `true|int`, `true|abs`
and `false|lower` give nothing (Jinja2: `1`, `1`, `false`); case `filters.filters_on_bool`.

**Done in 0027.** `dictsort` items unpack in `for k, v in ...` (the six `filters.dictsort*`
cases match), and every built-in filter now rejects arguments it does not declare. Filters
declare Jinja2's `trim(chars)`, `xmlattr(autospace)` and `groupby(default, case_sensitive)`
so valid calls do not fail, but still ignore them (`filters.trim_chars`, `filters.xmlattr`).

**Proposal.** Fix filter by filter against `jinja2/filters.py`, one PR per handful of
filters, each removing its lines from `divergences/`. `format` needs a printf-style
formatter with Python semantics (`%s` uses 0012's `str()`, `%(name)s` mappings).
Banker's rounding in `round` follows Python's `round()`.

**Scheduling.** `default` replacing None needs 0034; `dictsort`, `groupby`, `tojson`, `xmlattr` and `urlencode` cases only become visible after the dict literals of 0013.

**Done when.** No line of `test/parity/divergences/` names task 0019, and `ctest -R parity` passes.

**Done (PR, wave 4).** Every 0019 case matches. How each filter got there, and the
choices that are not Python's:

- `attr` no longer falls back to items for a dict; a user-provided map (reflected struct,
  JSON object) still exposes its keys as attributes (0048). `map(attribute=)`, `sort`,
  `groupby` and `select/rejectattr` share Jinja2's attribute getter: dotted paths, digit
  parts as indexes, `default`.
- `batch` and `slice` were swapped; both are ports of `do_batch`/`do_slice` now.
- `groupby` sorts, honours `default` and `case_sensitive`, and yields `(grouper, list)`
  namedtuples (`ListAdapter::MarkAsNamedTuple`): they unpack, index and print as tuples.
- `int`, `float`, `abs`, `round` follow Python on bools, strings (`int(s, base)` then
  `int(float(s))`), defaults `0`/`0.0`, and errors; `round` keeps ints int, rounds the exact
  binary value half to even, and checks `method`.
- `format` is printf-style (`src/python_format.cpp`, reusable for `str % x` in 0020) when the
  string contains `%`; without one it keeps Jinja2C++'s `{}` syntax (fmt) for
  compatibility, where Jinja2 would raise "not all arguments converted".
- `tojson` writes `json.dumps(sort_keys=True)` itself (ensure_ascii, `, `/`: `, `indent`
  as in Python) instead of going through the JSON binding.
- `xmlattr` prints ` key="escaped str(value)"` in mapping order, rejects bad keys and
  honours `autospace`; callables are still left out.
- `center`, `title`, `trim(chars)`, `striptags`, `truncate`, `urlencode` are ports of
  the Python code; string filters apply `str()` to non-strings first.

Leftovers (Unicode case mapping, the full HTML entity table, big integers, the unused
binding serializers) are task 0048.

