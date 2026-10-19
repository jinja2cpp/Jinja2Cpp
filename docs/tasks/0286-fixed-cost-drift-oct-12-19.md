---
status: open
priority: medium
area: perf
depends: [0205]
touches: [src/template_parser.cpp, src/template_impl.h, src/value_visitors.h, src/internal_value.h, src/value.cpp]
---
# Fixed-cost drift from feature merges, 2026-10-12 to 2026-10-19

**Problem.** The weekly scout of 2026-10-19 compared master 9cf8586 with 8f13350d (0158),
41 merges earlier. No perf work landed in that span except 0205. Each merge stayed under
the +3% preflight gate and under the 3% drift alert (0123), but together they added:

- **Load**: +2.6% to +5.3% on every case except chat_mistral and mitsuhiko_table_wide.
  Examples: expressions +5.3%, filters +4.1%, many_tags +3.9%, plain_text +3.9%.
- **Render**:
  - strings +6.8%
  - plain_text +5.5% (+56 Ir)
  - chat_mistral +5.2%, with allocations 360 to 448
  - substitute +3.7%, for_range +3.5%, dict_ops +2.2%, macros +2.0%, expressions +1.9%

Attribution from the `bench-data` trend:

| Merge | Load (median step) | Render |
|---|---:|---|
| f4e312c, train 8 (#501-#507; 0043 ordered ValuesMap) | +0.82% | chat_mistral +3.6% and +90 allocations, strings +2.0% |
| 941c91c, train 26 (public BigInt, C++-only filters) | +1.21% | chat_* +0.7..1.45% |
| 9cf8586, train 27 (FindUndeclaredVariables, 0236) | +1.00% | none |
| #543, #545, #559, #588 (parity fixes) | none | plain_text +1.4/+0.2/+1.0/+1.4%, inheritance up to +1.7% |
| #558 | -0.93% | strings +1.6% |
| 0168/0193 after 0205 | about +0.9% | none (accepted in 0205) |

Report: `/mnt/project-files/perf-track/2026-10-19-scout.md`.

**Approach.** Work as 0205 did: per-merge `bench/count.py` and callgrind diffs, the
largest steps first.

1. **Train 27, Load +1.0%.** 0236 says Load pays nothing, but plain_text has no `set` or
   `trans`. Find the frames in `template.cpp`/`template_impl.h` (an inlining or layout
   shift) or in the `ParseSet` frame change.
2. **Train 26, Load +1.2% and chat Render +1.4%.** Check whether the BigInt alternative
   or visitor arms changed the evaluator's hot dispatch (`value_visitors.h`), or the
   conversion of user data.
3. **Train 8, chat_mistral +90 allocations.** If the ordered ValuesMap (0043) costs one
   allocation per converted message dict, that is a 2.0 design cost. Measure it and say
   so. If another PR in the train is the cause, fix it.
4. **strings +6.8% and plain_text/substitute Render.** Trace the parity merges' per-render
   fixed cost.

Each fix must be behaviour-neutral. A cost that is the price of a feature is documented
here as accepted, with its Ir.

**Done when.** Each listed step is either recovered or documented as accepted with its
cost, and master's Load and Render counts are back within +1% of 8f13350d or the
remainder is listed.

**Process note.** Fixed costs below 3% per PR pass every gate today. The steward's train
run could ask the perf track for a verdict when the train moves any case past +0.5%. That
is Ruslan's call; it is not part of this task.
