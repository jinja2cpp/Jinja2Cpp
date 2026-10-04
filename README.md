# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (12 so far, latest [098d156](https://github.com/jinja2cpp/Jinja2Cpp/commit/098d1561eeccfda0dd999fb4e7789c1b74ef43f6) on 2026-10-04). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 79,489 | +0.13% | -46.21% | 88 | 0% | -32.82% |
| [Render/dict_ops](#renderdict_ops) | 482,827 | -6.54% | -30.11% | 323 | -2.42% | -28.22% |
| [Load/expressions](#loadexpressions) | 104,415 | -0.02% | -49.35% | 103 | 0% | -29.45% |
| [Render/expressions](#renderexpressions) | 635,587 | -11.71% | -29.86% | 10 | -44.44% | -66.67% |
| [Load/filters](#loadfilters) | 161,702 | -0.16% | -41.08% | 153 | 0% | -32.89% |
| [Render/filters](#renderfilters) | 81,845 | -0.77% | -7.70% | 63 | -4.55% | -14.86% |
| [Load/for_filter_if](#loadfor_filter_if) | 70,357 | +1.06% | -48.03% | 80 | 0% | -34.96% |
| [Render/for_filter_if](#renderfor_filter_if) | 668,260 | -5.34% | -22.79% | 210 | -3.67% | -7.08% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 88,068 | -0.08% | -51.25% | 92 | 0% | -41.40% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 785,795 | -5.66% | -26.75% | 507 | -1.36% | -2.87% |
| [Load/for_range](#loadfor_range) | 33,620 | +0.16% | -40.69% | 47 | 0% | -16.07% |
| [Render/for_range](#renderfor_range) | 67,348 | -12.22% | -26.96% | 10 | -33.33% | -62.96% |
| [Load/inheritance](#loadinheritance) | 58,844 | +0.10% | -48.44% | 68 | 0% | -36.45% |
| [Render/inheritance](#renderinheritance) | 670,639 | +0.18% | -10.32% | 565 | -1.22% | -9.16% |
| [Load/large_static](#loadlarge_static) | 603,480 | +0.06% | -31.89% | 244 | 0% | -44.55% |
| [Render/large_static](#renderlarge_static) | 30,987 | -55.26% | -58.76% | 4 | -60.00% | -77.78% |
| [Load/macros](#loadmacros) | 122,146 | -0.09% | -46.42% | 127 | 0% | -31.72% |
| [Render/macros](#rendermacros) | 1,417,684 | -3.03% | -44.69% | 412 | -2.37% | -84.90% |
| [Load/many_tags](#loadmany_tags) | 21,709,651 | +0.07% | -50.84% | 16,559 | 0% | -51.65% |
| [Render/many_tags](#rendermany_tags) | 1,865,881 | +0.28% | -10.42% | 5 | -61.54% | -76.19% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 123,480 | +0.21% | -48.02% | 133 | 0% | -27.72% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 9,600,426 | -12.94% | -22.30% | 3,030 | -25.09% | -25.24% |
| [Load/plain_text](#loadplain_text) | 9,575 | +0.32% | -1.04% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,651 | +0.65% | -60.98% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 154,618 | -0.12% | -47.02% | 142 | 0% | -36.61% |
| [Render/strings](#renderstrings) | 1,879,613 | -2.35% | -10.39% | 1,990 | -0.20% | -5.87% |
| [Load/substitute](#loadsubstitute) | 19,872 | +0.25% | -33.44% | 33 | 0% | -10.81% |
| [Render/substitute](#rendersubstitute) | 4,596 | +0.15% | -47.86% | 4 | 0% | -66.67% |

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
