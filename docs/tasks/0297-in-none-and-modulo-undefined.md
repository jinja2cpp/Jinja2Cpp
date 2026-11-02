---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/723
priority: low
area: parity
depends: []
touches: [src/expression_evaluator.cpp, src/python_format.cpp, src/testers.cpp#IsValueIn, test/parity/cases/operators.py, test/errors_test.cpp]
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

**Resolution.** `in` with None or a callable on the right is the TypeError (`testers::IsValueIn`, so
the `in` test too). `%` takes an undefined right operand for a mapping, as Python's
`PyMapping_Check` does through `Undefined.__getitem__`: no "not all arguments converted", `%(key)`
is an item use (an error, or ChainableUndefined itself), `%d`/`%f` are int()/float() use, `%s`
fails only for StrictUndefined and `%r` never; a string on the left no longer rejects a
StrictUndefined before its directives run. Ordering and arithmetic with an undefined operand
(`5 + nope`, `nope < 5`) are UndefinedError rather than TypeError, as Undefined's `__add__` and
`__lt__` raise. Cases in `operators.py`, error codes in `ErrorContractTest`. Callable type names in
these messages are 0368.
