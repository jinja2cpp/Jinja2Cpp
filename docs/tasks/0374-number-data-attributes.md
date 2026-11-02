---
status: open
priority: low
area: parity
touches: [src/value_methods.cpp#GetAttr, src/value_methods.cpp#IsMethodName, src/filters.cpp#Attribute]
shares: [test/parity/cases/methods.py, test/parity/divergences/methods.txt]
---
# int and float data attributes: real, imag, numerator, denominator

**Problem.** Found by the 0317 verifier. Python ints have the data attributes `real`, `imag`,
`numerator` and `denominator`, floats `real` and `imag`, and Jinja2's getattr finds them:
`{{ (3).real }}`, `{{ 2.5.imag }}`, `3|attr('real')` and `'{0.numerator}'.format(4)` render
`3`, `0.0`, `3` and `4`. Here an attribute of a number is undefined (or, in `str.format`,
an AttributeError), so the template fails or prints nothing
(`methods.number_data_attributes`). The method tables (`IntMethods`, `FloatMethods`) hold
callables only, and the parser asks `IsMethodName` before it takes the getattr path, so a data
attribute needs its own entry point, shared by `GetAttr`, `str.format`'s `FormatAttr` and the
`attr` filter. A bool is an int here too (`true.real` is `1`).

**Proposal.** A small table of number data attributes next to `IntMethods`/`FloatMethods`
(name, getter on int, big int, bool and double), consulted by `GetAttr` after the methods,
listed by `IsMethodName` so that the parser keeps the getattr path, and by `Attribute::Filter`.
`__class__` stays out of scope (no class objects).

**Done when.** `methods.number_data_attributes` matches and its line is gone from
`test/parity/divergences/methods.txt`.
