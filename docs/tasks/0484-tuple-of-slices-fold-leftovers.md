---
status: in-progress
priority: low
area: parity
depends: [0471]
touches: [src/expression_parser.cpp#ParseSubscript]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt, python/tests/test_fallback.py]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/880
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
  Jinja2 calls it once. Each such evaluation is bounded by the render limits, so the worst Load
  costs about the depth limit times a worst render of the tag (PR #865 security review);
- per render: Jinja2 replaces the folded subscript by its constant, while the engine's sealed
  tree keeps a TupleSliceExpression, which evaluates its literal operand and parts and builds
  the undefined's name each time it runs (in a loop, once per iteration). Caching the value
  needs per-template storage for the constants the Load's fold check computed, since sealed
  nodes are shared and immutable (Codex P2 on #865).

**Proposal.** Low value. Make the item or attribute of such a subscript a node whose evaluation
fails (as Jinja2's fold of it does), and let the Load's fold check (`FoldCheckPasses`) decide, as it
does for the subscript itself; or keep the line and its use as the fallback sample.

**Done when.** The cases match in narrow and wide runs and its divergence lines are gone; then
`NOPARSE` needs another syntax the engine lacks, or the tests that need one are dropped.

**Resolution (parity batch 7).**

- An item or attribute of the undefined a tuple of slices folds to is a plain subscript of it
  (ParseSubscript no longer rejects it). Its evaluation fails as Jinja2's fold of it does, so the
  Load's check (`FoldCheckPasses`) decides as for any candidate; under ChainableUndefined it is the
  undefined itself, also a tuple of slices of it (`TupleSliceExpression::Evaluate`).
- Every operator node (`and`/`or`, arithmetic, `~`, comparisons, `not`, unary `-`) is a fold
  candidate, as in Jinja2's optimizer. With StrictUndefined a candidate's failure fails the Load only
  where Jinja2's `as_const` lets the UndefinedError out (`and`, `or`, `~`, a conditional, an output);
  an arithmetic operator, a comparison, `not`, a subscript, a filter or a test gives Impossible
  (`FoldCandidate::failsLoad`). An output that folds is folded whole before the optimizer sees its
  parts (`visit_Output`), so under StrictUndefined the candidates inside it are not evaluated.
- Comparing lists, tuples and dicts compares their items with `==`, and `in` its items, which a
  StrictUndefined refuses; lists of different lengths are unequal without their items being compared.
- The Python module's `NOPARSE` sample is now `{% if [1][:, :]|sort|first %}{% endif %}` (0492).

Left for 0492: the fold of one step of a filter chain, the strict generator filters, the per-Load
cost under StrictUndefined and the per-render cost of a folded subscript.
