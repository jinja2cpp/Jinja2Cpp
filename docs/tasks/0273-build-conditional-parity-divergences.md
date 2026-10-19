---
status: done
priority: medium
area: parity
touches: [test/parity/parity_test.cpp, test/parity/update_divergences.py, test/parity/generate.py, test/parity/README.md]
---
# Build-conditional parity divergences (0178 phase 0)

**Problem.** A corpus case can diverge in one build and match in another (big integers with and
without a backend). The divergence lists have no way to say so.

**Proposal** (plan section 8). A divergence kind may carry a feature suffix, e.g.
`rejects@nobigint`: the case must diverge in builds without the feature and must match in builds
with it. The test binary reports its features in the results header; `update_divergences.py` keeps
suffixed lines and the report names them. Unknown suffixes fail.

**Done when.** `ParityRegistry` rows for a valid suffix, an unknown suffix and a missing kind pass,
and `update_divergences.py` self-checks on fixture results with and without the feature header.

Plan: [docs/design/0178-big-integer-plan.md](../design/0178-big-integer-plan.md).

Done in #595.
