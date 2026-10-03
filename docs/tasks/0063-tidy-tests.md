---
status: done
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

## Outcome

On master 850f797 the whole-tree run had 52 hits left in `test/` outside include-cleaner
(0064) and identifier naming (0065); all are fixed. `ApplyEnv` in
`test/parity/parity_test.cpp` (cognitive complexity 35, 17 brace hits) became two option
tables plus `ApplyExtensions`. `test/.clang-tidy` sets
`bugprone-unchecked-optional-access.IgnoreValueCalls`: `optional::value()` throws, which
fails the test, so it is the checked access in a test. `SUBSTITUTION_TEST_P` keeps its
bare `TestName` under `NOLINTBEGIN(bugprone-macro-parentheses)`: it is a declared name.
With `src/` clean since 0055, `bugprone-*` and `cppcoreguidelines-init-variables` enter
`WarningsAsErrors`.
