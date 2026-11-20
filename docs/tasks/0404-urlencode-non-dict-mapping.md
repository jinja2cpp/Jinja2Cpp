---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/963
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

**Result (parity batch 11 B).** The host seam has `IsDict` (`isinstance(x, dict)`) next to `IsMapping`, internal
(`IEngineHostObject`) and public (`jinja2::IHostObject`, an additive virtual whose default is `IsMapping()`, so
existing host mappings keep reading as dicts; the G1 freeze is postponed and additive API may merge). The
module's `PyHost::IsDict` is `PyDict_Check` (a subclass, `OrderedDict` and `defaultdict` are dicts). `urlencode`
takes the items of a host dict only; any other host mapping is iterated, which gives its keys to unpack, as
Jinja2's `do_urlencode`. Its unpack errors now carry Python's class (`ValueError` for too many or too few
values, `TypeError` for a non-iterable item), so the module raises what Jinja2 raises.
`python/tests/test_render.py::test_urlencode_takes_items_only_of_a_dict` covers `MappingProxyType`, a
`collections.abc.Mapping` subclass, dict subclasses and plain dicts; the docs/python.md line is removed.
