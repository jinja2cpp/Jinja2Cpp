---
status: open
priority: low
area: parity
depends: []
touches: [src/filters.cpp#Attribute, test/parity/cases/filters.py]
---
# `|attr` of a missing attribute under DebugUndefined prints empty

**Problem.** Found by the 0297/0307 verifier. `{{ 'x'|attr('f') }}` under DebugUndefined renders
`{{ no such element: str object['f'] }}` in Jinja2 3.1.6 and an empty string in Jinja2C++: the filter's
undefined result carries no name or object, so DebugUndefined has nothing to print. Not in the
corpus yet: `filters.py` is group A's area in parity batch 2, so the case is added with the fix.

**Proposal.** Build the result with `MakeUndefined(context, obj, key)`, as subscripts do.

**Done when** a `filters.attr_missing_debug` case (narrow and wide) matches Python.
