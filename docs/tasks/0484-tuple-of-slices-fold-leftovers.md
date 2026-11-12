---
status: open
priority: low
area: parity
depends: [0471]
touches: [src/expression_parser.cpp#ParseSubscript]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt, python/tests/test_fallback.py]
---
# Tuple-of-slices folds the Load check still decides differently from Jinja2

**Problem.** Since 0471 the engine keeps a subscript of a literal by a tuple of slices (`[1][:, :]`)
where Jinja2's optimizer folds an expression around it, and checks the fold at Load. An item or
attribute of it (`[1][:, :][0]`, `[1][:, :].x`) is still rejected while parsing: Jinja2's fold of
that item fails (an item of an undefined raises), so the item alone leaves invalid Python. But a
fold around it that does not evaluate it removes it: `{{ true or [1][:, :][0] }}` renders `True`
in Jinja2 (`subscripts.tuple_of_slices_item_not_evaluated`), and the engine rejects it. The Python
module's tests use this template as the syntax the engine lacks (`NOPARSE` in
`python/tests/test_fallback.py`).

The verifier of 0471 found more, pinned in `test/parity/cases/subscripts.py`:

- under ChainableUndefined an attribute or item of such a subscript folds too, `{{ [1][:, :].a }}`
  renders empty (`tuple_of_slices_chainable_item`), and the parser rejects it;
- with StrictUndefined Jinja2's optimizer folds every expression of the tree, so a strict
  undefined fails inside an `or` that a folded test skips (`tuple_of_slices_strict_short_circuit`);
  the engine evaluates only the candidates it records (ExpressionParser::AddFoldCandidate);
- comparing lists that hold a strict undefined raises in Python, not in the engine
  (`tuple_of_slices_strict_list_compare`);
- with StrictUndefined `[1][:, :]|batch(2)`, `|slice(2)` and `|unique` fold in Jinja2, whose
  filters return generators, and fail in the engine, which evaluates them eagerly (no case: the
  output is a generator's repr with its address);
- the candidates of a filter chain are the whole chain, so a chain whose last filter does not
  fold, or folds to an undefined, loses a fold of an earlier step (`[1][:, :] is undefined|host_filter`,
  `{% if [1][:, :]|sort|first %}`);
- binary operators, `~` and comparisons are no candidates: `{{ x or 'ab'[1:, 1] ~ ()[:, :] }}` and
  `{{ x == [1][:, :] ~ ()[:, :] }}` load in Jinja2; under ChainableUndefined so does
  `{{ [1][:, :][:, :] is undefined }}`;
- under StrictUndefined every candidate is evaluated and any failure fails the Load, but Jinja2's
  Filter and Test `as_const` turn an exception into Impossible: `{{ true if 1 else ()[:, :]|sum }}`
  and `{{ 1 or ()[:, :]|trim }}` load in Jinja2 and not in the engine; `{{ true in [none, ()[:, :]] }}`
  is the other way round;
- cost: under StrictUndefined nested brackets evaluate each level's candidate, so a chain nested
  to the depth limit is evaluated up to about 35 times (the default policy skips the candidates
  inside one that folds); a host filter in a candidate is called once per candidate it is in, where
  Jinja2 calls it once.

**Proposal.** Low value. Make the item or attribute of such a subscript a node whose evaluation
fails (as Jinja2's fold of it does), and let the Load's fold check (`FoldCheckPasses`) decide, as it
does for the subscript itself; or keep the line and its use as the fallback sample.

**Done when.** The cases match in narrow and wide runs and its divergence lines are gone; then
`NOPARSE` needs another syntax the engine lacks, or the tests that need one are dropped.
