---
status: open
priority: low
area: parity
depends: [0445]
touches: [src/expression_parser.cpp#Parse]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt, python/tests/test_fallback.py]
---
# A tuple of slices in a conditional that Jinja2 folds by its test

**Problem.** Since 0445 the engine loads a subscript of a literal by a tuple of slices
(`()[:, :]`) where Jinja2 folds the output it is in, and rejects it elsewhere, as Jinja2's
unfolded code is invalid Python. It decides with `Expression::IsFoldable` at load. A conditional
whose test folds (`1 > 0`, a comparison of literals) is foldable only "by choice"
(`PrepareFold` gives `FoldByChoice`, `IsFoldableAt` decides at render), so the engine rejects

    {{ (()[:, :] if 1 > 0) is undefined and '' }}

while Jinja2 folds the test, picks the first branch and renders `''`
(`subscripts.tuple_of_slices_fold_by_choice`). The Python module's tests use this template as
the syntax the engine lacks (`NOPARSE` in `python/tests/test_fallback.py`).

**Proposal.** Low value. At load, when an output holding such a subscript folds only by a
choice, evaluate the choice there (the tests that decide it are literals and pure filters, so
they need no render), or keep the line and its use as the fallback sample.

**Done when.** The case matches in narrow and wide runs and its divergence line is gone; then
`NOPARSE` needs another syntax the engine lacks, or the tests that need one are dropped.
