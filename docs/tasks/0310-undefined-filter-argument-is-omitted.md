---
status: open
priority: low
area: parity
touches: [src/function_base.h, src/string_converter_filter.cpp]
---
# A filter argument given as an undefined value reads as omitted

**Problem.** Found by the 0107 verifier. A filter reads an argument the template did not pass
as undefined, the same value an argument passed as an undefined variable has, so the two
cannot be told apart. Jinja2 passes the Undefined object itself: `{{ 'xyx'|trim(nope) }}`
raises `TypeError: strip arg must be None or str` in Python (`filters.trim_undefined_chars`)
and renders `xyx` here, as `trim()` does. Other filters with optional string arguments
(`center`'s fill aside, `join`'s separator, `indent`'s width, `urlize`'s `rel`/`target`)
likely share it.

**Proposal.** Let `FunctionBase` report whether an argument was given (or hand filters an
"absent" marker distinct from undefined), then make each filter apply Python's type check to
a given undefined. Sweep the filters with a parity case per optional argument.

**Done when.** `filters.trim_undefined_chars` matches and its line is gone from
`test/parity/divergences/filters.txt`.
