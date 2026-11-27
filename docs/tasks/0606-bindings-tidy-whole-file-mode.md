---
status: open
priority: medium
area: ci
depends: [0601]
touches: [.github/workflows/bindings.yml]
---
# clang-tidy ratchet for the bindings job: it still reports changed lines only

**Problem.** Task 0601 moves the pull-request clang-tidy job and `scripts/preflight.sh` to whole changed
files (`scripts/tidy_changed.sh`). The `bindings clang-tidy (changed lines)` job in `bindings.yml` stays on
`clang-tidy-diff.py`: it covers `bindings/` and `test/binding/` in a configuration that gives every component
its include path, which `clang-tidy.yml` (nlohmann only) does not have. The same anchoring gaps apply there
(a cognitive-complexity score, an include line, a moved hit), and no weekly job covers the Boost bindings, so
a hit there is never reported on the whole file.

**Proposal.** Run `scripts/tidy_changed.sh` in that job over the bindings paths (it already excludes the
Boost components that this configuration cannot analyse by name; the job's own exclusion of
`bindings/boost_multiprecision/*` and the `test/big_int_test.cpp` coverage stay), and first clear what the
whole-file run reports on the current tree. A weekly whole-tree run of the bindings configuration is the
other half; decide with 0601's outcome whether it belongs in `clang-tidy.yml` or in `bindings.yml`.

**Done when.** A PR that adds a branch to a bindings function at cognitive score 25 fails the bindings job,
and the bindings tree is clean in that configuration.
