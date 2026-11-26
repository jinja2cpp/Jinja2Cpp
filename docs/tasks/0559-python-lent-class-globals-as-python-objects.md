---
status: in-progress
priority: low
area: parity
depends: [0439]
touches: [python/src/convert.cpp#ToPythonImpl, python/src/convert.cpp#Classify]
shares: [include/jinja2cpp/template.h#DescribeLentCallable, python/tests/test_context.py, docs/python.md]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1014
---
# Python module: a lent `range` is not Python's `range`

**Problem.** 0439 made a template callable lent to a Python callable print as Jinja2's object does
(`jinja2::DescribeLentCallable`: `<class 'range'>`, `<Macro 'm'>`). Jinja2 passes its own objects, though:
`Template('{{ f(range) }}').render(f=lambda x: type(x).__name__)` gives `type` in Jinja2 and `TemplateValue` through
the module, and `f=lambda x: x is range` gives True in Jinja2 and False. The same holds for `dict`, `cycler`
(`jinja2.utils.Cycler`), `joiner`, `namespace` and `lipsum` (`jinja2.utils.generate_lorem_ipsum`); a macro is a
`jinja2.runtime.Macro` there, which the module has no object for.

**Proposal.** Hand Python its own object for the engine's class globals and `lipsum` (the description's
`LentCallableInfo` would need the global's `__module__`/`__qualname__`), and map those Python objects back to the
engine's globals where Python returns them to the template, so that `{{ f(range) is sameas range }}` (True in both
today, through the lent callable) stays True. Weigh the cost of the check on the conversion of every Python callable.

**Done when.** `type(x)` and `x is range` match Jinja2 in a `python/tests/test_context.py` row, the identity row
stays green, and the `docs/python.md` sentence naming 0559 is gone.
