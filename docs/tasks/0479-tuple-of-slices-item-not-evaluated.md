---
status: open
priority: low
area: parity
depends: [0471]
touches: [src/expression_parser.cpp#ParseSubscript]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt, python/tests/test_fallback.py]
---
# An item of a tuple-of-slices subscript that Jinja2's fold never evaluates

**Problem.** Since 0471 the engine keeps a subscript of a literal by a tuple of slices (`[1][:, :]`)
where Jinja2's optimizer folds an expression around it, and checks the fold at Load. An item or
attribute of it (`[1][:, :][0]`, `[1][:, :].x`) is still rejected while parsing: Jinja2's fold of
that item fails (an item of an undefined raises), so the item alone leaves invalid Python. But a
fold around it that does not evaluate it removes it: `{{ true or [1][:, :][0] }}` renders `True`
in Jinja2 (`subscripts.tuple_of_slices_item_not_evaluated`), and the engine rejects it. The Python
module's tests use this template as the syntax the engine lacks (`NOPARSE` in
`python/tests/test_fallback.py`).

**Proposal.** Low value. Make the item or attribute of such a subscript a node whose evaluation
fails (as Jinja2's fold of it does), and let the Load's fold check (`FoldCheckPasses`) decide, as it
does for the subscript itself; or keep the line and its use as the fallback sample.

**Done when.** The case matches in narrow and wide runs and its divergence lines are gone; then
`NOPARSE` needs another syntax the engine lacks, or the tests that need one are dropped.
