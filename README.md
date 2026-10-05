# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (15 so far, latest [b7eacec](https://github.com/jinja2cpp/Jinja2Cpp/commit/b7eacec4b185d990a384663fd4b7b7c2a8d947e9) on 2026-10-05). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 81,952 | 0% | -44.54% | 88 | 0% | -32.82% |
| [Render/dict_ops](#renderdict_ops) | 477,117 | 0% | -30.94% | 323 | 0% | -28.22% |
| [Load/expressions](#loadexpressions) | 105,101 | 0% | -49.02% | 103 | 0% | -29.45% |
| [Render/expressions](#renderexpressions) | 640,405 | 0% | -29.33% | 10 | 0% | -66.67% |
| [Load/filters](#loadfilters) | 162,817 | 0% | -40.67% | 153 | 0% | -32.89% |
| [Render/filters](#renderfilters) | 81,198 | 0% | -8.43% | 63 | 0% | -14.86% |
| [Load/for_filter_if](#loadfor_filter_if) | 71,049 | 0% | -47.52% | 80 | 0% | -34.96% |
| [Render/for_filter_if](#renderfor_filter_if) | 667,672 | 0% | -22.86% | 210 | 0% | -7.08% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 89,084 | 0% | -50.69% | 92 | 0% | -41.40% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 786,515 | 0% | -26.68% | 507 | 0% | -2.87% |
| [Load/for_range](#loadfor_range) | 34,294 | 0% | -39.50% | 47 | 0% | -16.07% |
| [Render/for_range](#renderfor_range) | 67,516 | 0% | -26.78% | 10 | 0% | -62.96% |
| [Load/inheritance](#loadinheritance) | 59,416 | 0% | -47.93% | 68 | 0% | -36.45% |
| [Render/inheritance](#renderinheritance) | 503,272 | 0% | -32.70% | 315 | 0% | -49.36% |
| [Load/large_static](#loadlarge_static) | 605,346 | 0% | -31.68% | 244 | 0% | -44.55% |
| [Render/large_static](#renderlarge_static) | 30,994 | 0% | -58.75% | 4 | 0% | -77.78% |
| [Load/macros](#loadmacros) | 122,930 | 0% | -46.07% | 127 | 0% | -31.72% |
| [Render/macros](#rendermacros) | 1,429,036 | 0% | -44.25% | 412 | 0% | -84.90% |
| [Load/many_tags](#loadmany_tags) | 22,094,790 | 0% | -49.97% | 16,559 | 0% | -51.65% |
| [Render/many_tags](#rendermany_tags) | 1,861,634 | 0% | -10.63% | 5 | 0% | -76.19% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 126,636 | 0% | -46.69% | 133 | 0% | -27.72% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 9,627,497 | 0% | -22.08% | 3,030 | 0% | -25.24% |
| [Load/plain_text](#loadplain_text) | 9,581 | 0% | -0.98% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,656 | 0% | -60.91% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 158,266 | 0% | -45.77% | 142 | 0% | -36.61% |
| [Render/strings](#renderstrings) | 1,898,241 | 0% | -9.50% | 1,990 | 0% | -5.87% |
| [Load/substitute](#loadsubstitute) | 19,951 | 0% | -33.18% | 33 | 0% | -10.81% |
| [Render/substitute](#rendersubstitute) | 4,599 | 0% | -47.82% | 4 | 0% | -66.67% |

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
