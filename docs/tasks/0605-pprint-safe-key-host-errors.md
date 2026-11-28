---
status: done
priority: low
area: parity
depends: [0582]
touches: [src/serialize_filters.cpp#SafeKeyLess, include/jinja2cpp/exception.h, python/src/adapters.cpp]
shares: [python/tests/test_values.py, test/public_host_object_test.cpp]
---
# pprint's `_safe_key` with host objects: catch only the TypeError, also inside a tuple key

**Problem.** Python's `pprint._safe_key` falls back to the type order only on a `TypeError` of `<`. Task 0582's
`SafeKeyLess` (src/serialize_filters.cpp) cannot tell a host's `TypeError` from its other errors: the Python module
throws every Python exception as `nb::python_error`, so the engine catches all of them when a key is a host object.
Two differences from Jinja2 3.1.6 follow:

- A host `__lt__` that raises something else (`ValueError('boom')`, `KeyboardInterrupt`) is swallowed:
  `{{ {a: 1, b: 2}|pprint }}` renders where Jinja2 raises. No memory-safety impact, but it hides a Ctrl-C and real
  host errors.
- A tuple key holding an incomparable host object (`{(1, f): 1, (1, 'a'): 2}` with `f = Fraction(7, 2)`) raises
  `TypeError: '<' not supported between instances of 'str' and 'Fraction'`, where Jinja2 orders by type: only a
  top-level host key enables the fallback.

**Plan.** A public mix-in next to `jinja2::StopIteration` (`include/jinja2cpp/exception.h`), say
`jinja2::HostTypeError`, that a host exception derives from to say "this is Python's TypeError"; the Python module
throws it for a `TypeError` (as `PyStopIteration` does), and `SafeKeyLess` catches only it and the engine's own
`TemplateRuntimeError`, at any depth of the key. Additive public API.

**Done when.** Both rows above render as Jinja2 does in `python/tests/test_values.py`, with a C++ row in
`test/public_host_object_test.cpp` for a host exception that is not the mix-in.
