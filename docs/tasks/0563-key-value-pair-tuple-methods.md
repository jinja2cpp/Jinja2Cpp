---
status: done
priority: low
area: parity
depends: []
touches: [src/value_methods.cpp#FindMethod]
shares: [test/parity/cases/filters.py, test/parity/divergences/filters.txt]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/988
---
# A dictsort or items pair has no tuple methods

**Problem.** The pairs `dictsort` and `items` give are Python tuples: `(d|dictsort|first).index('a')` is `0`,
`.count(1)` is `1`, and `.count` alone prints the bound method (Jinja2 3.1.6). The engine's pair is a
`KeyValuePair` value, which `FindMethod` gives no methods, so the calls fail with "'tuple object' has no attribute
'index'" and `.count` is undefined (case `filters.dictsort_pair_methods`, found by the 0545 verifier).

**Proposal.** Look a KeyValuePair's attributes up in `TupleMethods`, as a two-item tuple, sandbox policy included.

**Done when** the divergence line is gone and the case matches in narrow and wide runs.

**Result.** A KeyValuePair has its own `index` and `count` over (key, value), found through `FindMethod` like a
tuple's, so the sandbox policy is asked as for any method (`PublicSandboxPolicyTest.PairMethodsAskThePolicy`). Cases
`filters.dictsort_pair_*` cover the bound method's repr, index ranges and the errors.
