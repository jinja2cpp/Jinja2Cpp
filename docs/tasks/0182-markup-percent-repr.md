---
status: open
priority: low
area: parity
depends: []
touches: [src/markup.h#EscapeFormatArgs, src/python_format.cpp]
shares: [src/expression_evaluator.cpp#ApplyPercentFormat, src/serialize_filters.cpp]
---
# `%r` and containers in Markup's `%`

**Problem.** Markup's `%` (and the `format` filter on Markup) escapes each argument before
formatting it (`EscapeFormatArgs` in `src/markup.h`), so `%r` sees the escaped Markup and
prints `Markup('x')`, and a container argument prints its escaped items' reprs. markupsafe
wraps each argument in `_MarkupEscapeHelper`, whose `__repr__` is `escape(repr(obj))` and
`__str__` is `escape(str(obj))`: the repr or str of the original, escaped as a whole
(`autoescape.markup_percent_repr`):

```
{{ ('%r'|safe) % 'x' }}|{{ ('%s'|safe) % {'a': '<'} }}
```

Jinja2 gives `&#39;x&#39;|{&#39;a&#39;: &#39;&lt;&#39;}`, Jinja2C++ `Markup('x')|{'a': Markup('&lt;')}`.
Before 0051 printed Markup's repr, the first gave `'x'`, also wrong.

**Proposal.** Keep the arguments as they are and let `PythonPercentFormat` escape each
conversion's text (`%s`, `%r`, `%a`) when the format is Markup, leaving numbers formatted
by `%d`/`%f` alone, as markupsafe's helper does.

**Done when.** `autoescape.markup_percent_repr` matches and its line is gone from
`test/parity/divergences/autoescape.txt`.
