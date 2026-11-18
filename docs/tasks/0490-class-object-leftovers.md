---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/940
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

## Resolution

1. **Fixed.** Every class object has an `operator()` that `CallPythonClass` (global_functions.cpp) answers: `int`, `float`, `str`,
   `bool`, `list`, `tuple`, `NoneType`, `type(x)`, `Markup`, and the four undefined classes build their value as Python's
   constructor does, through the engine's own conversions (`PythonIntOfText`, `PythonFloatOfText`, the `int`, `float`, `list`,
   `string` and `safe` filters, `ConvertToBool`); their errors carry Python's class and message (`ValueError` for a bad literal,
   `TypeError` for too many arguments or a type with no number, `OverflowError` text for `int(inf)`). Any other class fails
   with `cannot create 'dict_keys' instances`. The class check sits on the path that already raised
   `'type' object is not callable` (`ResolveCallee` finds `operator()` as it does for `joiner()`), so a call of a macro or
   function pays nothing. `is callable` follows from the same slot. Deliberate limits: `int(x, base)` of a base other than
   10 reads the text through the `int` filter, which also accepts a float literal there; `Undefined(hint, obj, name)` binds its
   arguments as Python does (an unknown or repeated keyword is a `TypeError`), but takes the hint as text only and ignores `exc`; classes Python can build with arguments that the engine has no value for (`reversed`, `function`,
   `method`, `LoopContext`, `object()`) report `cannot create` where Python reports a different `TypeError` or builds one.
2. **Fixed.** `__mro__` and `__bases__` give the tuple of classes, `mro()` a new list, with an `object` class and the parents
   bool to int, Markup to str, `StrictUndefined`, `ChainableUndefined` and `DebugUndefined` to `Undefined`, `_GroupTuple` to
   tuple. They are read at the access (`ClassObjectAttribute`), not stored, so a class does not hold itself. The sandbox
   policy is asked for them by name as for `__class__`.
3. **Fixed.** groupby's groups are `jinja2.filters._GroupTuple`, `lipsum(n)` is `Markup` (not with `html=False`), and
   `self.blk` and `super` are `jinja2.runtime.BlockReference` (a macro-kind callable without the `arguments` attribute that
   every macro has, so no new callable kind).
4. **Fixed.** `FormatAttr` answers `__class__` of an undefined before the policy check, so `'{0.__class__}'.format(zzz)` prints
   the class under every policy; any other attribute still fails as before.
5. **Fixed.** A namespace's `__class__` is a hidden slot as `operator()` is: `ns['__class__']` and `ns.__class__` give the
   class, whatever `{% set ns.__class__ = 5 %}` stored.
6. **Deliberate.** With autoescape Python fails to print the `Markup` and `TemplateModule` classes (`Markup.__html__()` unbound);
   the engine prints them escaped. Printing a class is not something a template needs, and a failure would be a new error path
   for no value.
7. **Deliberate.** A class object is a dict-like copy that `update`/`clear` change, as cycler, joiner and loop are; Python
   refuses `update`. Making it immutable needs a read-only map kind, which no other object has either.
8. Documented in sandbox_policy.h; no change.
