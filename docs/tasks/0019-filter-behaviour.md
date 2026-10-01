---
status: open
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
int and rounds half away from zero; `slice` chunks like `batch`; `sort` ignores
`attribute='a,b'`; `striptags` keeps newlines; `title` keeps inner capitals; `tojson`
uses compact separators; `trim` ignores `chars`; `truncate` measures length differently
and has no `leeway`; `urlencode` quotes like `quote_plus`; `join` rejects `d=`.

**Proposal.** Fix filter by filter against `jinja2/filters.py`, one PR per handful of
filters, each removing its lines from `divergences.txt`. `format` needs a printf-style
formatter with Python semantics (`%s` uses 0012's `str()`, `%(name)s` mappings).
Banker's rounding in `round` follows Python's `round()`.

**Scheduling.** `default` replacing None needs 0034; `dictsort`, `groupby`, `tojson`, `xmlattr` and `urlencode` cases only become visible after the dict literals of 0013.

**Done when.** No line of `test/parity/divergences.txt` names task 0019, and `ctest -R parity` passes.
