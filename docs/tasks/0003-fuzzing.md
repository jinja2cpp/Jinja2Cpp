---
status: open
priority: high
area: robustness
issues: ["#287", "#288"]
---
# Continuous fuzzing of lexer/parser/evaluator

**Problem.** Templates are often user-supplied input, yet nothing exercises malformed
input systematically. Open issues #287 and #288 came from external fuzzing; there is no
in-repo harness to reproduce or regress them.

**Proposal.**
1. `fuzz/` with libFuzzer targets: parse-only (`Template::Load`) and parse+render with a
   small fixed context. Seed corpus from `test/` templates and the parity corpus (0001).
2. CI: a short time-boxed run (clang, `-fsanitize=fuzzer,address,undefined`) on PRs that
   touch `src/`; longer runs on a schedule. Crashes become regression tests.
3. Consider OSS-Fuzz once the targets are stable.

**Done when.** #287 and #288 have regression tests; the PR fuzz job runs green.
