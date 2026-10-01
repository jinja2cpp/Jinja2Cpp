---
status: open
priority: high
area: parity
depends: [0001, 0012]
touches: []
shares: [src/filters.cpp, src/filters.h, src/string_converter_filter.cpp]
---
# Missing builtin filters

**Problem.** Templates using `string`, `safe`, `indent`, `count`, `e`, `filesizeformat`,
`forceescape`, `items` or `urlize` fail at render time with "Can't find filter". `string`,
`safe` and `indent` are among the most used filters in real templates (20 cases).

**Proposal.** Add each filter with Jinja2's signature and defaults (`indent(width=4,
first=False, blank=False)`, `filesizeformat(binary=False)`, `urlize(trim_url_limit,
nofollow, target, rel, extra_schemes)`). `string` uses the conversion from 0012; `safe`
and `forceescape` need the markup flag from 0025 but can land first as pass-through and
escape. `count` and `e` are aliases of `length` and `escape`.

**Done when.** No line of `test/parity/divergences.txt` names task 0018, and `ctest -R parity` passes.
