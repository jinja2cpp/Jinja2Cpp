---
status: open
priority: low
area: perf
depends: []
touches: [src/template_impl.h, src/render_workspace.h, src/render_workspace.cpp, src/render_context.h]
---
# Tiny templates: the last places another engine is ahead

**Problem.** The 2026-10-12 engine comparison (master 587e5ed, median of 3 `bench/run.py`
runs, /mnt/project-files/perf-track/2026-10-12-engines.md) has Jinja2C++ ahead of MiniJinja
on every Render case, and ahead of Tera 2 on 9 of 11. What is left is fixed cost on tiny
templates:

| Benchmark | Jinja2C++ | Tera 2 | MiniJinja |
|---|---:|---:|---:|
| `Render/substitute` | 215 ns | 165 ns | 443 ns |
| `Render/plain_text` | 93 ns | 91 ns | 329 ns |
| `Load/strings` | 8.38 µs | | 7.07 µs |
| `Load/plain_text` | 360 ns | 1.4 µs | 319 ns |
| `Load/config_file` | 9.98 µs | | 8.98 µs |

`Render/substitute` (`{{ message }} from Parser! - {{ number }}`, 2,208 instructions) spends
its time outside the two lookups. `callgrind_annotate`, self cost:
- `ValueRefExpression::EvaluateRef` 15.5%
- `TemplateImpl::Render` 12.3%
- the user `ValuesMap` robin_hood lookup 11.3%
- `RenderWorkspace::EmptyMaps` 8.6%, which clears the lent tables even when nothing was
  written; this is 0160's lazy-lend idea
- `TemplateRenderer::RenderBody` 6.2%
- a `std::string` construction per render 3.3%
- `RenderContext` construction and destruction 5.4%

`Load/strings` is 13% in `TagLexer::LexToEnd`. The rest is spread across the parser, the
arena moves and filter-name lookup (`FindBuiltinFilter` 1.9%).

**Proposal.** Profile `Render/substitute` and `Render/plain_text` first. Make the workspace
lend lazy, so `EmptyMaps` runs only for tables a render wrote. Find and drop the per-render
string. Fold `RenderContext` setup into `TemplateImpl::Render`. Then look at `Load/strings`
for per-tag costs that MiniJinja does not pay.

**Done when.** `Render/substitute` is at or under 1,700 instructions (-23%, about Tera's
time) and `Render/plain_text` is under 900, with no Render case worse than +0.3%. Load is a
stretch goal: `Load/strings` within 5% of MiniJinja's time.
