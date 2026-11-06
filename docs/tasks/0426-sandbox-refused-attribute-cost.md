---
status: open
priority: low
area: perf
depends: [0354]
touches: [python/src/adapters.cpp, python/jinja2cpp/sandbox.py]
shares: [python/tests/test_sandbox.py]
---
# Cost of a refused attribute in the sandbox

**Problem.** The sandbox's answer for a private name of an engine value is built from a Python stand-in
object (`Representative`, `Stub`) and a call to `_sandbox_attribute`. Measured by the verifier of 0354:
7 to 15 microseconds per refused private attribute, where Jinja2's own refusal costs 0.1. `Representative()`
copies a string value each time (about 0.5 ms per refusal on a 5 MB string), and `Stub()` builds a new class
per call. A template that probes many private names pays it each time; nothing else does (the check is not on
the path of public names unless `is_safe_attribute` is overridden).

**Proposal.** Cache the stub classes by name for the life of the environment; for a string, pass an empty one
of the same type (the default `is_safe_attribute` looks at the type and name only), and the real one only when
`is_safe_attribute` is overridden. Consider a per-render cache of the answer by (kind, name) when the method is
not overridden.

**Done when.** `bench` or a pytest timing guard shows a refused private name on a 5 MB string under 20
microseconds, with the existing sandbox tests unchanged.
