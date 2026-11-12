---
status: in-progress
priority: low
area: parity
depends: [0445]
touches: [src/expression_parser.cpp#Parse]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt, python/tests/test_fallback.py]
---
# Tuple-of-slices subscripts: folds the engine decides differently from Jinja2

**Problem.** Since 0445 the engine loads a subscript of a literal by a tuple of slices
(`()[:, :]`) where Jinja2 folds the output it is in, and rejects it elsewhere, as Jinja2's
unfolded code is invalid Python. It decides with `Expression::IsFoldable` at load. A conditional
whose test folds (`1 > 0`, a comparison of literals) is foldable only "by choice"
(`PrepareFold` gives `FoldByChoice`, `IsFoldableAt` decides at render), so the engine rejects

    {{ (()[:, :] if 1 > 0) is undefined and '' }}

while Jinja2 folds the test, picks the first branch and renders `''`
(`subscripts.tuple_of_slices_fold_by_choice`). The Python module's tests use this template as
the syntax the engine lacks (`NOPARSE` in `python/tests/test_fallback.py`).

The verifier of 0445 found more places where Jinja2's folding and the engine's load-time
`IsFoldable` disagree (all pinned in `test/parity/cases/subscripts.py`):

- the tuple of slices in the test of such a conditional: `{{ 1 if [1][:, :] is undefined }}`
  renders `1` (`tuple_of_slices_in_test`);
- Jinja2's optimizer folds such a conditional in `{% set %}` too, and in a loop filter:
  `{% set q = 1 if true else ()[:, :] %}` loads (`tuple_of_slices_set_by_choice`);
- `{% print ()[:, :] %}` builds an Output that Jinja2 folds; the engine folds only in `{{ }}`
  (`tuple_of_slices_print`);
- with StrictUndefined Jinja2's fold fails wherever the undefined is stringified, compared or
  tested for truth (`~`, `or`, `and`, `|length`, `not`, `==`, a branch of a conditional), so those
  are load errors; the engine refuses only a bare output of it (`tuple_of_slices_strict_concat`);
- `()[0][:, :]`: `()[0]` folds to an undefined, whose subscript does not fold, so Jinja2 writes
  invalid Python; the engine folds it again (`tuple_of_slices_of_folded_undefined`).
- a part whose evaluation raises (`[1 / 0][:, :]`, `()[:, 1 / 0]`): Jinja2's fold fails, so it
  rejects at load; the engine checks only that the parts are foldable and renders empty
  (`tuple_of_slices_failing_part`, Codex on #845).

**Proposal.** Low value. At load, when an output holding such a subscript folds only by a
choice, evaluate the choice there (the tests that decide it are literals and pure filters, so
they need no render), or keep the line and its use as the fallback sample.

**Done when.** The cases match in narrow and wide runs and their divergence lines are gone; then
`NOPARSE` needs another syntax the engine lacks, or the tests that need one are dropped.
