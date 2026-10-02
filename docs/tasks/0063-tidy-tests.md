---
status: open
priority: low
area: style
depends: [0057, 0070]
touches: [test/]
---
# clang-tidy on test/

**Problem.** `test/.clang-tidy` (inherits the root config, relaxes six checks for gtest
code) reports about 110 hits in test files, the largest `readability-qualified-auto` (69)
and `google-explicit-constructor` (40). test/ has hand-aligned tables (0009), so whole-file
rewrites are off limits.

**Proposal.** Apply fixes only where they do not touch the aligned tables; the PR job
already checks touched test lines. Run it when no parity wave edits test tables.

**Done when** the whole-tree job reports zero hits in `test/`.
