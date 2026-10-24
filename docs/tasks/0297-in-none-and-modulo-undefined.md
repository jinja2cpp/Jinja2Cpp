---
status: open
priority: low
area: parity
depends: []
touches: [src/expression_evaluator.cpp, test/parity/cases/operators.json]
---
# `x in None` and `'text' % undefined` differ from Python

**Problem.** The 0289 verifier's random differential found two mismatches. Base 631c9ed9 has
both, so they are not caused by the concat change. Checked against Jinja2 3.1.6:

| Template | Python | Jinja2C++ |
|---|---|---|
| `{{ 1 in none }}` | `TypeError: argument of type 'NoneType' is not iterable` | `False` |
| `{{ "xy" % undef_x }}` | `xy` (an Undefined argument is consumed and ignored) | render error |
| `{{ "x%sy" % undef_x }}` | `xy` | `xy` (already matches) |

An undefined right operand of `in` (`{{ 1 in z }}`) gives `False` in both, so only None (and
other non-containers, to check) needs the error.

**Proposal.** Make `in` with a right operand that is not a container, string or Undefined fail as
Python's TypeError does (0255 error codes). Let `%` with no conversion in the format string
accept a single Undefined argument the way Python does.

**Done when** parity cases for the rows above (narrow and wide) match the pinned Python, with
no new allow-list lines.
