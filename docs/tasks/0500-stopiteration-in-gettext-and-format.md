---
status: open
priority: low
area: parity
depends: [0479]
touches: [src/global_functions.cpp, src/statements.cpp#TransStatement, src/value_methods.cpp]
shares: [python/tests/test_i18n.py, python/tests/test_sandbox.py]
---
# A StopIteration out of gettext or str.format is undefined in Jinja2

**Problem.** Left by 0479 (found by the 0477 verifier). 0479 gives undefined for a StopIteration out of a macro,
`caller()`, `loop(...)` or a callable the template calls, but not for calls the engine makes through CallValue or
its own string methods:
- `_`, `gettext`, `ngettext` and `{% trans %}` with `install_gettext_callables` whose callable raises StopIteration:
  Jinja2 renders `''` for old-style `_`/`gettext` and a `trans` without variables, and fails with UndefinedError
  ("value was undefined because a callable raised a StopIteration exception") for newstyle gettext or a `trans`
  with variables (`Undefined % vars`). The engine calls gettext through CallValue (src/global_functions.cpp), so
  the render ends with `RuntimeError: generator raised StopIteration` through the module.
- In a sandbox, `'{a.p}'.format_map(d)` / `.format(...)` where the property `p` raises StopIteration, or a
  `wrap_str_format` that does: Jinja2 renders `''`.

**Proposal.** Guard those invocations with the 0479 classifier (src/call_stop.h) after their arguments are
evaluated, and make the i18n formatting of an undefined result raise as Jinja2's does.

**Done when.** Each case above matches Jinja2 through the module, with rows in python/tests/test_i18n.py and
python/tests/test_sandbox.py.
