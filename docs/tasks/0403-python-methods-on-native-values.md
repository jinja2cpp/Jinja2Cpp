---
status: open
priority: low
area: parity
depends: [0353]
touches: [src/value_methods.cpp]
shares: [python/tests/test_values.py, docs/python.md]
---
# Python methods of int, float and str the engine does not implement

**Problem.** Jinja2 lets a template call any Python method of an `int`, `float` or `str`:
`f.hex()`, `'ab'.encode()`, `n.bit_length()`. The engine implements a fixed set of methods for
its own values, so the rest raise `TemplateRuntimeError`. This holds for values from Python
and for template literals alike, so it is not a module-only gap (found by the 0353 verifier,
listed in docs/python.md "Differences").

**Proposal.** List the methods Jinja2's sandbox-free environment can reach on these types,
pick the ones chat and config templates use (start from the parity corpus and the bench
cases), and implement them in `src/value_methods.cpp` with parity cases. Methods returning
`bytes` stay unsupported and are recorded as a divergence.

**Acceptance.** Each implemented method matches Python Jinja2 in a parity case, narrow and wide;
the rest are listed in docs/python.md with the reason.
