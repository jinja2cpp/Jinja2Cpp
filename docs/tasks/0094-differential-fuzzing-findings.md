---
status: open
priority: medium
area: parity
depends: [0003]
touches: [test/parity/cases/fuzz.py, test/parity/divergences/fuzz.txt]
---
# Divergences found by differential fuzzing

**Problem.** The parity corpus (0001) only knows the templates someone wrote into it. The
first run of the differential check (`fuzz/differential.py`, task 0003) rendered the
libFuzzer render corpus, about 1900 inputs mutated from the parity cases and unit tests,
with both engines and found about 100 divergences the corpus does not cover. Most of them
are known classes already owned by other tasks: `ValuesMap` key order (0043), undefined
printed as `None` in containers (0047), big integers (0015), Markup string methods,
HTML5 entities in `striptags`, string escapes `\x`, `\u`, `\N{}`, `\a`. The ones below were
new. Each is a case in `test/parity/cases/fuzz.py`, listed in
`test/parity/divergences/fuzz.txt`:

- Iterating a value Python cannot iterate renders nothing instead of raising TypeError:
  `{% for i in true %}`, `true|join`.
- Filter arguments outside their domain are ignored: `dictsort(by='bogus')` (Python raises
  FilterArgumentError), `'x'|format(1)` and the same on a Markup string (TypeError "not all
  arguments converted"), `wordwrap(2.0)` with a word longer than the width (TypeError).
- `{% include [x, 'header.j2'] %}` with a non-string first name fails; Python's
  `select_template` skips it and includes `header.j2`.
- `{% do l.append(l) %}` fails: a list cannot contain itself (value semantics).
- Syntax Jinja2 rejects that Jinja2C++ accepts: `{{ 'a' +}}` (the `+` is taken as a
  whitespace-control marker), `{% macro m %}` and `{% call m %}` without parentheses, and
  `{'a' = 1}` dict literals. The last three are old Jinja2C++ leniencies the unit tests
  use; whether to keep them is a decision (Jinja2C++ extensions, documented) or a fix
  (reject like Jinja2, and update those unit tests).

Fixed in the PR that found them: a trailing comma in call arguments, `f(a, )` and
`is test(x,)`, was a parse error (case `literals.call_trailing_comma`).

**Proposal.** Fix each in the area that owns the behaviour and move its case to that
area's file. Decide the syntax leniencies first: they are the only part that can break
existing templates.

**Done when.** `test/parity/divergences/fuzz.txt` is empty, or lists only the leniencies
kept as documented extensions.

**Next.** The nightly differential job (`.github/workflows/fuzz.yml`) keeps finding inputs
like these; its report is the queue for this task.
