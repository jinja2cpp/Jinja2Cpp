---
status: done
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

**Resolution.** Python 3.12's public methods of the three types, against the engine:
- str: added `format_map` (0443), `expandtabs` (0452), `translate` and `maketrans` (dict,
  list or str tables; Markup's `translate` returns Markup and does not escape the table, as
  markupsafe's). Not added: `encode` (returns `bytes`).
- int and bool: added `bit_count`, `conjugate`, `as_integer_ratio`, `is_integer` (3.12); had
  `bit_length`. Not added: `to_bytes`, `from_bytes` (`bytes`).
- float: added `hex`, `conjugate`, `as_integer_ratio` (a denominator past int64 is a big int);
  had `is_integer`. Not added: `fromhex`, an alternate constructor rather than a method of a
  value.

Each added method has a corpus case in cases/methods.py (narrow and wide); the three left out
are deliberate lines in divergences/methods.txt and are listed in docs/python.md.
