# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (13 so far, latest [29c6c73](https://github.com/jinja2cpp/Jinja2Cpp/commit/29c6c733b7bd7f9929971bcac2fff4aaa23a0b4c) on 2026-10-04). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 79,897 | +0.51% | -45.93% | 88 | 0% | -32.82% |
| [Render/dict_ops](#renderdict_ops) | 476,763 | -1.26% | -30.99% | 323 | 0% | -28.22% |
| [Load/expressions](#loadexpressions) | 104,602 | +0.18% | -49.26% | 103 | 0% | -29.45% |
| [Render/expressions](#renderexpressions) | 635,808 | +0.03% | -29.83% | 10 | 0% | -66.67% |
| [Load/filters](#loadfilters) | 161,826 | +0.08% | -41.03% | 153 | 0% | -32.89% |
| [Render/filters](#renderfilters) | 81,835 | -0.01% | -7.71% | 63 | 0% | -14.86% |
| [Load/for_filter_if](#loadfor_filter_if) | 70,127 | -0.33% | -48.20% | 80 | 0% | -34.96% |
| [Render/for_filter_if](#renderfor_filter_if) | 667,533 | -0.11% | -22.87% | 210 | 0% | -7.08% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 87,988 | -0.09% | -51.30% | 92 | 0% | -41.40% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 785,915 | +0.02% | -26.74% | 507 | 0% | -2.87% |
| [Load/for_range](#loadfor_range) | 33,736 | +0.35% | -40.48% | 47 | 0% | -16.07% |
| [Render/for_range](#renderfor_range) | 67,468 | +0.18% | -26.83% | 10 | 0% | -62.96% |
| [Load/inheritance](#loadinheritance) | 58,986 | +0.24% | -48.31% | 68 | 0% | -36.45% |
| [Render/inheritance](#renderinheritance) | 504,366 | -24.79% | -32.56% | 315 | -44.25% | -49.36% |
| [Load/large_static](#loadlarge_static) | 604,570 | +0.18% | -31.76% | 244 | 0% | -44.55% |
| [Render/large_static](#renderlarge_static) | 30,994 | +0.02% | -58.75% | 4 | 0% | -77.78% |
| [Load/macros](#loadmacros) | 122,085 | -0.05% | -46.44% | 127 | 0% | -31.72% |
| [Render/macros](#rendermacros) | 1,419,622 | +0.14% | -44.61% | 412 | 0% | -84.90% |
| [Load/many_tags](#loadmany_tags) | 21,782,801 | +0.34% | -50.68% | 16,559 | 0% | -51.65% |
| [Render/many_tags](#rendermany_tags) | 1,865,864 | 0% | -10.42% | 5 | 0% | -76.19% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 124,264 | +0.64% | -47.69% | 133 | 0% | -27.72% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 9,627,461 | +0.28% | -22.08% | 3,030 | 0% | -25.24% |
| [Load/plain_text](#loadplain_text) | 9,575 | 0% | -1.04% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,656 | +0.19% | -60.91% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 155,236 | +0.40% | -46.81% | 142 | 0% | -36.61% |
| [Render/strings](#renderstrings) | 1,898,925 | +1.03% | -9.47% | 1,990 | 0% | -5.87% |
| [Load/substitute](#loadsubstitute) | 19,896 | +0.12% | -33.36% | 33 | 0% | -10.81% |
| [Render/substitute](#rendersubstitute) | 4,599 | +0.07% | -47.82% | 4 | 0% | -66.67% |

## Charts

### Load/dict_ops

![Load/dict_ops](charts/Load_dict_ops.svg)

### Render/dict_ops

![Render/dict_ops](charts/Render_dict_ops.svg)

### Load/expressions

![Load/expressions](charts/Load_expressions.svg)

### Render/expressions

![Render/expressions](charts/Render_expressions.svg)

### Load/filters

![Load/filters](charts/Load_filters.svg)

### Render/filters

![Render/filters](charts/Render_filters.svg)

### Load/for_filter_if

![Load/for_filter_if](charts/Load_for_filter_if.svg)

### Render/for_filter_if

![Render/for_filter_if](charts/Render_for_filter_if.svg)

### Load/for_loop_vars

![Load/for_loop_vars](charts/Load_for_loop_vars.svg)

### Render/for_loop_vars

![Render/for_loop_vars](charts/Render_for_loop_vars.svg)

### Load/for_range

![Load/for_range](charts/Load_for_range.svg)

### Render/for_range

![Render/for_range](charts/Render_for_range.svg)

### Load/inheritance

![Load/inheritance](charts/Load_inheritance.svg)

### Render/inheritance

![Render/inheritance](charts/Render_inheritance.svg)

### Load/large_static

![Load/large_static](charts/Load_large_static.svg)

### Render/large_static

![Render/large_static](charts/Render_large_static.svg)

### Load/macros

![Load/macros](charts/Load_macros.svg)

### Render/macros

![Render/macros](charts/Render_macros.svg)

### Load/many_tags

![Load/many_tags](charts/Load_many_tags.svg)

### Render/many_tags

![Render/many_tags](charts/Render_many_tags.svg)

### Load/mitsuhiko_table

![Load/mitsuhiko_table](charts/Load_mitsuhiko_table.svg)

### Render/mitsuhiko_table

![Render/mitsuhiko_table](charts/Render_mitsuhiko_table.svg)

### Load/plain_text

![Load/plain_text](charts/Load_plain_text.svg)

### Render/plain_text

![Render/plain_text](charts/Render_plain_text.svg)

### Load/strings

![Load/strings](charts/Load_strings.svg)

### Render/strings

![Render/strings](charts/Render_strings.svg)

### Load/substitute

![Load/substitute](charts/Load_substitute.svg)

### Render/substitute

![Render/substitute](charts/Render_substitute.svg)
