---
status: open
priority: medium
area: perf
touches: [.github/workflows/benchmark.yml, bench/trend.py]
shares: [bench/README.md]
---
# Count `--data reflect` in the PR gate and the trend

**Problem.** `bench/count.py --data reflect` passes each case's data through the nlohmann
binding (`jinja2::Reflect`), so every lookup goes through a user `IMapItemAccessor`.
CI never runs it. `.github/workflows/benchmark.yml` runs only the default `--data
convert`, both in the `instructions` gate and in the `trend` job.

As a result, claims about the reflection path go unchecked:
- #658 (0311 S1+S2) claimed reflect gains of -2.1% to -11.4%. They came from local runs
  against an old base, and its CI job could not confirm them. The scout of 2026-10-26
  confirmed them by hand, at -2.7% to -11.5% from 778e37a (before #658) to master,
  with convert counts within ±0.4% over the same span. That check took two extra
  builds.
- The 0049 P2 thread (#673) measures host-data rows locally.

A regression on this path also passes every gate. 0115 and 0311 both change it.

The path is far from free. Scout of 2026-10-26, master 0620aca, reflect against convert,
Render:

| Case | Instructions | Allocations |
|---|---:|---:|
| `dict_ops` | +158% | 112 to 113 |
| `for_filter_if` | +151% | 4 to 1,082 |
| `inheritance` | +90% | 35 to 335 |
| `chat_qwen` | +83% | 238 to 749 |
| `chat_llama` | +73% | 251 to 676 |
| `mitsuhiko_table` | +56% | 33 to 5,026 |
| `config_file` | +51% | 908 to 2,422 |
| `html_autoescape` | +49% | 674 to 2,091 |
| `macros` | +42% | 308 to 908 |

Cases without data are unchanged (`expressions`, `for_range`, `plain_text`, ...), and
wide cases still convert.

**Proposal.**
1. **Trend.** After the default run, the `trend` job runs `count.py --data reflect
   --filter '^Render/'` (about 5 s more). It stores the counts in the same
   `history.jsonl` record under names such as `Render/chat_llama@reflect`, so
   `trend.py render` draws them and `trend.py drift` alerts on them like any other row.
   Skip cases whose reflect counts equal their convert counts (no data, or wide), so the
   charts do not double.
2. **PR gate.** The `instructions` job runs the same reflect pass for base and head and
   reports the comparison in the step summary. It does not fail the job at first. Make it
   gate (3%, as for convert) once a few weeks of trend show it is as repeatable as the
   default mode (it should be: same binary, same callgrind).
3. Note in `bench/README.md` (Trend) that `@reflect` rows exist and what they measure.

**Done when.** The trend record of the merge that lands this has `@reflect` rows. A PR
that touches `include/jinja2cpp/reflected_value.h` or a binding shows a reflect
comparison in its gate summary.

**Note.** The brief for this scout said CI runs neither `--data reflect` nor `--data
convert`. `convert` is the default, so every gate and trend run already counts it. Only
`reflect` is missing.
