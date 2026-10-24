---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/647
priority: low
area: parity
depends: []
touches: [src/string_converter_filter.cpp]
---
# String filter divergences found by the 0061 differential

**Problem.** The differential run that verified the 0061 split of `StringConverter::Filter`
(8462 templates, narrow and wide, autoescape on and off) found no change from the split,
but several differences that master has too:

- `urlencode` of a non-ASCII wide string renders an empty string; the narrow string is
  percent-encoded as UTF-8.
- Against Python Jinja2 3.1.6 about 830 of 4800 non-error results differ per autoescape
  setting, spread over replace, safe, urlize, center, trim, truncate, title and wordcount.
- C++ accepts arguments Python rejects: `wordwrap(0)`, `wordwrap(-3)`,
  `replace('a', 'b', 'c')`, `truncate(10, true, 1)`, `indent(2.5)`, `trim(1)`,
  `urlize(extra_schemes='ftp')`.

**Proposal.** Fix the wide `urlencode` first (it loses data). Then turn the rest into parity
cases under `test/parity/cases/` by filter, and fix or list each in
`test/parity/divergences/`. The generator and harness from the run are worth adding to
`test/parity/` so the corpus covers argument errors, Markup inputs and non-string inputs.

**Also (found by the untrusted-input review, 2026-10-18).** `('x'*3)|replace('', 'y')` returns
the input unchanged; Python gives `yxyxyxy`. The method form is covered (test/parity/cases/methods.py),
the filter form is not.

**Done when** wide `urlencode` matches narrow and each class of difference is a corpus case
that matches Python or is listed as deliberate.

**Outcome.** Wide `urlencode` already matched narrow (`filters.urlencode_non_ascii` pins it).
`replace` now takes the str() of `old` and `new`, inserts around every character for an empty
`old`, counts as `str.replace` does (`0` replaces none, `none` or a negative count all, a
float raises) and caps its result; `indent` refuses a float width, `trim` a non-string
`chars`, `urlize` a string `extra_schemes`, `wordwrap` a non-string value. `wordwrap(0)`,
`wordwrap(-3)` and `truncate(10, true, 1)` already raised. The differential lives on as the
`string_filters` parity area: 27 calls on 10 kinds of input, autoescape off and on, all matching.
