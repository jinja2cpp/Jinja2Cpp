---
status: open
priority: low
area: parity
depends: [0476]
touches: [src/internal_value.cpp#ClassObjectAdapter, src/value_methods.cpp#FormatAttr, src/expression_evaluator.cpp#CallExpression]
shares: [test/parity/cases/literals.py, test/parity/divergences/literals.txt]
---
# Leftovers of 0476: what the engine's class objects still lack

**Problem.** Found by the verifier of the 0475/0476 PR (Jinja2 3.1.6). The class `x.__class__` gives
(0476) differs from Python's:

1. Only the classes that are globals (`range`, `dict`, `cycler`, `joiner`, `namespace`) are
   callable, being those globals. The rest are not: `{{ (1).__class__ is callable }}` is `False`
   (Python `True`), and `{{ (1).__class__() }}` fails with `'type' object is not callable` where
   Python prints `0` (`(1).__class__('5')`, `zzz.__class__()` likewise).
2. `__mro__`, `__bases__` and `mro` are undefined (Python: a tuple of classes, a bound method).
3. Some values name another class than Python's: groupby's groups are `tuple` (Python
   `jinja2.filters._GroupTuple`), `self.blk` and `super` are `Macro` (Python `BlockReference`),
   `lipsum(1)` is `str` (Python `Markup`).
4. `'{0.__class__}'.format(zzz)` fails for an undefined argument in the default, strict and debug
   policies and prints `''` under chainable; Python prints the undefined's class (`FormatAttr`
   uses the undefined before its fields).
5. `{% set ns.__class__ = 5 %}{{ ns['__class__'] }}` prints `5`; Python still gives the class.
6. With autoescape, Python fails to print the `Markup` and `TemplateModule` classes
   (`Markup.__html__()` unbound); the engine prints them escaped. Record deliberate.
7. A class object is a dict-like copy that `update`/`clear` change, as cycler, joiner and loop are;
   Python refuses `update`.
8. A host `ISandboxPolicy` that leaves `__class__` alone (`nullopt`) now lets templates read the
   class (`__name__`, `__module__`, chained `__class__`); Jinja2's `is_safe_attribute` refuses it.
   Policies that refuse underscore names, as Jinja2's does, are unaffected. Documented in
   sandbox_policy.h.

**Proposal.** 1: a class that calls the engine's conversion for the builtins it has (`int`, `str`,
`float`, `bool`, `list`, `tuple`, `dict`) and is callable to `is callable`; the rest fail with
Python's message. 2-5: add when a template needs them; pin each with a case. 6, 7: record deliberate.

**Done when** each item is fixed or recorded deliberate, with a corpus case per fixed item.
