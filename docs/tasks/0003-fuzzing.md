---
status: done
priority: high
area: robustness
issues: ["#287", "#288"]
touches: [fuzz/, .github/workflows/fuzz.yml, src/expression_parser.cpp, src/expression_parser.h, src/recursion_guard.h]
shares: [CMakeLists.txt, src/statements.cpp, src/template_impl.h, include/jinja2cpp/error_info.h, test/errors_test.cpp]
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

**Also (found in 0014).** The recursive-descent parser has no depth limit: about 1000
nested `(`, `[`, `not` or unary `-` in one expression overflow the stack (ASan
stack-overflow; 600 is fine). Jinja2 raises `RecursionError` there; Jinja2C++ should
return a parse error past a fixed depth instead of crashing.
Unbounded template recursion does the same: `{% macro m() %}{{ m() }}{% endmacro %}{{ m() }}`
overflows the stack, and so does a user `gettext` macro that contains `{% trans %}` (found
by task 0029); Jinja2 raises `RecursionError`.

**Done when.** #287 and #288 have regression tests; the PR fuzz job runs green.

**Outcome.** `fuzz/` holds three libFuzzer targets (parse; load and render with includes,
imports and extends; the same through `TemplateW`), a seed corpus built from the parity
cases and unit-test templates, a dictionary, crash regressions replayed by ctest under
every compiler, and a differential check against Python Jinja2 (`fuzz/differential.py`).
`.github/workflows/fuzz.yml` fuzzes 5 minutes per target on pull requests and an hour
nightly, keeping the corpus in the Actions cache. #287 and #288 were already fixed; their
inputs are in `fuzz/regressions/`. The depth limits above are in `src/recursion_guard.h`:
expression nesting (256), chained operators per statement (1024) and render recursion
(256: macros, `caller()`, `super()`, `self.<block>`, recursive loops, include, import,
extends) end in `ErrorCode::RecursionLimitExceeded` instead of a stack overflow. Python
Jinja2 stops earlier on each, so no template it renders is rejected. The first fuzzing
rounds (about 2.6M executions) found nothing else; the differential check found the
divergences filed as 0090.

**Next.**
- OSS-Fuzz or ClusterFuzzLite, once the nightly job has run clean for a while: longer
  runs, crash deduplication and coverage reports for free.
- A grammar-aware mutator (libprotobuf-mutator or a Jinja2 grammar for AFL++/Nautilus):
  byte mutation rarely produces long well-formed templates, so deep features (nested
  blocks with macros and loops) are reached mostly through the seeds.
- Resource limits: `range(10**9)` or `'x' * 10**9` run unbounded. Jinja2's sandbox caps
  `range` at 100000; Jinja2C++ has no sandbox mode yet.
- The render depth is counted per thread, but the stack it guards is whatever the caller's
  thread has: 256 nested macro calls fit the 8 MiB main stack under ASan, not a 512 KiB
  secondary thread on macOS. A `Settings` field for the limit would let embedders match
  their stack size.
