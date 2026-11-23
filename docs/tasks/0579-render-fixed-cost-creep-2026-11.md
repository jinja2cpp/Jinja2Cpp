---
status: open
priority: low
area: perf
depends: [0578]
touches: [src/render_context.h, src/template_impl.h, src/statements.cpp, src/recursion_guard.h]
---
# Render rows crept +1-2% in the week to 2026-11-23, in steps under the bar

**Problem.** Found by the 2026-11-23 perf scout (`/mnt/project-files/perf-track/2026-11-23-scout.md`).
On the CI trend (`bench-data`), from 37c378c (2026-11-16) to 5e3c63f (= master 784eab1's engine),
32 merges, no single step above +1% and the drift job (3%) silent on these rows:

| Row | Week | Steps of 0.2% or more |
|---|---:|---|
| Render/inheritance | +1.99% | #927 +0.34, #940 +0.24, #953 +0.48, #957 +0.27, #959 +0.21, #976 +0.22, #979 -0.22 |
| Render/chat_llama | +1.23% | #916 +0.33, #927 +0.23, #940 +0.54, #946 -0.47, #957 +0.22, #973 +0.29, #990 +0.33 |
| Render/hf_gpt_oss | +1.09% | #932 +0.27, #940 +0.80 (0578), #946 -0.41 |
| Render/macros | +1.01% | #928 +0.27, #940 +0.28, #973 +0.45, #976 -0.43, #980 +0.53 |
| Render/plain_text | +2.21% (951 to 972 Ir) | #927 +9, #928 +5, #932 -8, #940 +5, #957 +10 (accepted for #955) |
| RenderSink/plain_text | +4.00% | the same steps; the 7-day drift anchor (0520) flagged it at dafffa9 |

Frames that grew, callgrind of 9541b58 against 784eab1, per render:
- `Render/inheritance` +2,246 Ir (+1.67%): `RenderIncludedTemplate` self +1,150, a now out-of-line
  `RenderContext::ExitScope` +510, `AllocateCountedBlock` +234 (0508's scope-map charge),
  `FindLoadedBy` +199.
- `Render/plain_text` +21 Ir: `RenderCore` self +19, `RenderContext::RenderContext` +9 (with
  `TakeTopScope` -9 folded in).

**Proposal.** As 0519 did for the module call: profile each step above +0.2% on these rows, move
cold setup out of the per-render path (one branch per render, as #830/#881), force-inline
`ExitScope` again, and get each row back within +0.5% of 37c378c. 0578 takes the #940 and #956
loop costs first.

**Done when.** The rows in the table are within +0.5% of 37c378c in `bench/count.py` (plain_text
within +10 Ir), no new allocations, and no other row above +0.3%.
