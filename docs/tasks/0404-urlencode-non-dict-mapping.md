---
status: open
priority: low
area: parity
depends: [0353]
touches: [src/string_converter_filter.cpp#UrlEncode]
shares: [python/tests/test_render.py, docs/python.md]
---
# urlencode of a Python Mapping that is not a dict

**Problem.** Jinja2's `do_urlencode` treats only a `dict` as a query string of its items; any
other `Mapping` is iterated, which gives its keys. The module reads every host mapping as a
dict (`src/string_converter_filter.cpp`, the `IsMapping` check), so
`{{ mapping_proxy|urlencode }}` differs (found by the 0353 verifier, in docs/python.md
"Differences").

**Proposal.** Let the host seam say whether a mapping is exactly `dict` (an `IsDict` next to
`IsMapping`, or a flag on it) and take the items branch only for that; other mappings iterate.
Engine maps (template dicts, JSON) keep today's behaviour, which matches Jinja2 for `dict`.

**Acceptance.** A pytest case with `types.MappingProxyType` and a `collections.abc.Mapping`
subclass matches Jinja2; the docs/python.md line is removed.
