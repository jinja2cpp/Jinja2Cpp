---
status: in-progress
priority: low
area: parity
depends: [0559]
touches: [python/src/convert.cpp#Classify, src/internal_value.cpp#Value2IntValue]
shares: [include/jinja2cpp/template.h, python/tests/test_context.py, docs/python.md]
---
# Python module: Python's `range` given to a template outside a lent call is not the template's `range`

**Problem.** 0559 hands a Python callable Python's own object for the engine's class globals and `lipsum`
(`builtins.range`, `jinja2.utils.Cycler`, ...), and maps that object back to the template's global while the call
it was lent to runs, so that `{{ f(range) is sameas range }}` is True. Anywhere else the object is a Python host
object: a render argument (`render(r=range)`, then `{{ r is sameas range }}`), or one a callable kept and gives back
in a later call (`python/tests/test_context.py::test_python_range_given_back_after_the_call_stays_pythons`). In
Jinja2 both are True, as the template's `range` is Python's. The template still calls it, as Python's.

**Proposal.** A public way to name the engine's class global as a host value that outlives a call (say
`jinja2::PythonGlobal(module, qualname)` returning a `UserCallable` that `Value2IntValue` turns back into the global),
which `Classify` uses for the six objects, by a pointer compare against objects cached once. Weigh the check on
every conversion of a Python object that is no common type, and what a host other than Python sees when it calls it.

**Done when.** The test row above prints `True` for `back() is sameas range`, `render(r=range)` with
`{{ r is sameas range }}` gives True, and the docs/python.md sentence naming 0583 is gone.
