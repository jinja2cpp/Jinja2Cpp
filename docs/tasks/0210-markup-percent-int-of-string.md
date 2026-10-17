---
status: open
priority: low
area: parity
depends: []
touches: [src/python_format.cpp]
---
# `%d` and `%f` of a string in Markup's `%`

**Problem.** markupsafe wraps each argument of Markup's `%` in `_MarkupEscapeHelper`, which
has `__int__` and `__float__` that call `int(obj)` and `float(obj)`. So `%d` and `%f` accept
a string that `int()` or `float()` parses (`autoescape.markup_percent_d_of_string`):

```
{{ ('%d|%.1f'|safe) % ('5', '1.5') }}
```

Jinja2 gives `5|1.5`, and `('%d'|safe)|format('5')` gives `5` too; Jinja2C++ raises "%d format: a real number is required, not str", as
Python does for a plain str format. 0182 left it: the conversion needs Python's `int()` and
`float()` string parsing (whitespace, underscores, Unicode digits, `inf`/`nan`), which
`python_format.cpp` does not have.

**Proposal.** When the format is Markup, convert a string argument of `%d`/`%i`/`%u` with
`int()` rules and of `%e`/`%f`/`%g` with `float()` rules, sharing the parser of the `int`
and `float` filters if it can be made to raise where Python raises.

**Done when.** `autoescape.markup_percent_d_of_string` matches and its line is gone from
`test/parity/divergences/autoescape.txt`.
