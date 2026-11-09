---
status: open
priority: low
area: perf
depends: []
touches: [src/expression_evaluator.cpp, src/undefined.cpp, src/filters.cpp]
---
# Sandbox checks cost nothing when no policy is installed

**Problem.** #801 (0354 PR 2) added sandbox arms to hot paths. A render without a policy
still pays for them. Scout 2026-11-09 (master 8a573d2,
/mnt/project-files/perf-track/2026-11-09-scout.md) measured:

- `if (idx.sandboxed)` in `SubscriptExpression::LookupDefinedIndex`
  (src/expression_evaluator.cpp:410-418) costs about 6.4 Ir per subscript when not taken.
  `hf_deepseek_v31` pays +405 Ir, although its preset sets no policy.
- The policy check in `MakeUndefined` (src/undefined.cpp:111) costs +924 Ir on `config_file`.
- These calls are out of line and run without a flag check first:
  - `SandboxedSyntheticAttribute` (src/filters.cpp:527);
  - `SandboxedNativeAttribute` in `MakeUndefinedAttr` (src/undefined.cpp:136);
  - the missing-method branch (src/expression_evaluator.cpp:1692).

#801 was approved at its rows:
- `hf_deepseek_v31` +0.49%
- `config_file` +0.43%
- `filters` +0.33%
- `hf_mistral_small` +0.28%
- `strings` +0.22%
- `chat_mistral` +0.20%

**Proposal.**
- Move every sandbox arm into a `[[gnu::cold]]` noinline helper, so the hot function keeps
  only the flag or pointer test.
- Gate the three out-of-line calls on `context.GetSandboxPolicy()`, as src/filters.cpp:512
  already does.

**Done when.** `bench/count.py` shows the rows above back to their pre-#801 counts, within
+0.1%. The sandbox tests still pass.
