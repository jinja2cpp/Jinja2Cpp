---
status: open
priority: low
area: parity
depends: [0484]
touches: [src/expression_parser.cpp#ParseFiltersAndTests]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt, python/tests/test_fallback.py]
---
# A tuple-of-slices fold of one step of a filter chain, and the costs of those folds

**Problem.** Left by 0484. Jinja2's optimizer folds each filter of a chain on its own, but the
engine's fold candidate is the whole chain (ExpressionParser::ParseFiltersAndTests). A chain whose
last filter does not fold, or folds to an undefined, loses the fold of an earlier step:
`{% if [1][:, :]|sort|first %}y{% endif %}` renders `[]` in Jinja2 (`[1][:, :]|sort` folds to `[]`),
and the engine rejects it (`subscripts.tuple_of_slices_chain_step`). The Python module's tests use
this template as the syntax the engine lacks (`NOPARSE` in `python/tests/test_fallback.py`,
`test_chat_templates.py`). Also left:

- with StrictUndefined `[1][:, :]|batch(2)`, `|slice(2)` and `|unique` fold in Jinja2, whose
  filters return generators, and fail in the engine, which evaluates them eagerly (no case: the
  output is a generator's repr with its address);
- a host filter in a chain, `[1][:, :] is undefined|host_filter`, the same as the chain above;
- with StrictUndefined, FoldCandidate::failsLoad is decided by the candidate's node kind, so a
  conditional does not pass on the Impossible of a filter in a branch:
  `{% if false and ([1][:, :]|length if 1 else x) %}a{% else %}b{% endif %}` renders `b` in Jinja2
  and fails the engine's Load; and `{% if false and (1 if [1][:, :] else x) %}` fails Jinja2's
  compile (the test's truthiness of a strict undefined) where the engine renders `b` (the verifier
  of 0484; both as before 0484);
- cost: under StrictUndefined every candidate is evaluated, and since 0484 every operator node
  around a tuple-of-slices subscript is a candidate, so a chain of N operators evaluates about N^2/2
  nodes at Load (N is bounded by Settings::Limits::maxExpressionOperators, 491), and nested brackets
  evaluate each level's candidate; a host filter in a candidate is called once per candidate it is
  in, where Jinja2 calls it once. Each evaluation is bounded by the render limits;
- per render: Jinja2 replaces the folded subscript by its constant, while the sealed tree keeps a
  TupleSliceExpression that evaluates its operand and parts and builds the undefined's name each
  time it runs. Caching it needs per-template storage for the constants the Load's fold check
  computed, since sealed nodes are shared and immutable (Codex P2 on #865).

**Proposal.** Record each step of a filter chain as a candidate (a FilteredExpression per step, or
a candidate that evaluates a prefix of the chain), within the Load perf gate. For the cost, evaluate
a candidate's inner candidates only where the outer one fails and stop at the first fatal failure.

**Done when.** `subscripts.tuple_of_slices_chain_step` matches in narrow and wide runs and its
divergence line and module `match` line are gone; `NOPARSE` needs another syntax or its tests are
dropped.
