---
status: in-progress
priority: low
area: parity
depends: [0434]
touches: [python/jinja2cpp/environment.py]
shares: [python/tests/test_values.py]
---
# A StopIteration out of a render is Jinja2's RuntimeError

**Problem.** A filter, test or `finalize` that raises StopIteration ends a Jinja2 render with
`RuntimeError: generator raised StopIteration`: Jinja2 renders through a generator, which PEP 479
turns the StopIteration into. The module raises the StopIteration itself. (A call in the template is
different: Jinja2's `Context.call` makes it an undefined value, which the module does since 0434.)
Found by 0434; `test_stop_iteration_from_a_call_is_undefined` in python/tests/test_values.py accepts
either class for now.

**Proposal.** Where `render`, `generate` and the module's other entry points re-raise the engine's
error, raise a StopIteration that escapes as `RuntimeError("generator raised StopIteration")` from it,
as Python does. Coordinate with 0457 (lazy generate), which owns `generate`.

**Done when.** Both engines raise RuntimeError for `{{ 1|stop }}` and the test asserts that class only.
