---
status: done
priority: low
area: parity
depends: [0159]
touches: [src/value_visitors.h, src/testers.cpp, src/expression_evaluator.cpp, src/internal_value.cpp]
---
# A dictsort pair still differs from a tuple in `in`, `==`, tests and truthiness

**Problem.** 0159 made `p[0]`, `p[-1]`, iteration, `|length`, `|list` and unpacking work on the
`KeyValuePair` that dictsort returns, but it is still a distinct variant, not a tuple-marked
list. With `p = {'ab': 1}|dictsort|first` the verifier found these diverging from Python:
`'ab' in p` and `1 in p` are false (Python true); `p == ('ab', 1)` is false and `!=` true;
`p is sequence` and `p is iterable` are false and `p is mapping` is true (Python true, true,
false); `{% if p %}` takes the else branch; `p[0:1]` renders empty (Python `('ab',)`);
`p + p` and `p * 2` error; `p.key` gives the key where Python gives undefined (kept: the
`['key']`/`['value']` reads are a Jinja2C++ extension that tests rely on).

**Proposal.** Prefer making dictsort return real tuples (as `dict.items()` does, via
`MakeTuple`) and keep a fast path for `for k, v in ...`; this fixes every row above at once
and costs the `key`/`value` extension and the `[k: v]` pprint form (existing tests in
`filters_test.cpp`, `expressions_test.cpp`, `ordered_map_test.cpp` encode them). Measure the
dictsort loop benchmark first. The alternative is to teach the contains, equality, tester,
truthiness and slice visitors about `KeyValuePair`.

**Done when** the corpus cases `filters.dictsort_pair_in`, `_equals`, `_tests` and `_truthy`
match and their lines in `test/parity/divergences/filters.txt` are deleted.
