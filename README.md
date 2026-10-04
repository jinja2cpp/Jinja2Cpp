# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (9 so far, latest [65f7a31](https://github.com/jinja2cpp/Jinja2Cpp/commit/65f7a3195fc756c8f0166eb4238c98a45f658bbc) on 2026-10-04). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 79,389 | -46.19% | -46.28% | 88 | -32.82% | -32.82% |
| [Render/dict_ops](#renderdict_ops) | 515,559 | -0.48% | -25.37% | 331 | 0% | -26.44% |
| [Load/expressions](#loadexpressions) | 104,432 | -49.15% | -49.34% | 103 | -29.45% | -29.45% |
| [Render/expressions](#renderexpressions) | 719,894 | 0% | -20.55% | 18 | 0% | -40.00% |
| [Load/filters](#loadfilters) | 161,969 | -40.44% | -40.98% | 153 | -32.89% | -32.89% |
| [Render/filters](#renderfilters) | 82,737 | -0.08% | -6.69% | 66 | 0% | -10.81% |
| [Load/for_filter_if](#loadfor_filter_if) | 69,616 | -48.37% | -48.58% | 80 | -34.96% | -34.96% |
| [Render/for_filter_if](#renderfor_filter_if) | 705,932 | +0.10% | -18.44% | 218 | 0% | -3.54% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 88,129 | -51.14% | -51.22% | 92 | -41.40% | -41.40% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 832,931 | -4.72% | -22.35% | 514 | 0% | -1.53% |
| [Load/for_range](#loadfor_range) | 33,567 | -40.85% | -40.78% | 47 | -16.07% | -16.07% |
| [Render/for_range](#renderfor_range) | 76,720 | -0.05% | -16.80% | 15 | 0% | -44.44% |
| [Load/inheritance](#loadinheritance) | 58,788 | -48.40% | -48.48% | 68 | -36.45% | -36.45% |
| [Render/inheritance](#renderinheritance) | 669,419 | -1.35% | -10.49% | 572 | 0% | -8.04% |
| [Load/large_static](#loadlarge_static) | 603,145 | -31.58% | -31.92% | 244 | -44.55% | -44.55% |
| [Render/large_static](#renderlarge_static) | 69,263 | -0.05% | -7.82% | 10 | 0% | -44.44% |
| [Load/macros](#loadmacros) | 122,252 | -46.28% | -46.37% | 127 | -31.72% | -31.72% |
| [Render/macros](#rendermacros) | 1,461,954 | 0% | -42.96% | 422 | 0% | -84.54% |
| [Load/many_tags](#loadmany_tags) | 21,695,366 | -51.13% | -50.88% | 16,559 | -51.65% | -51.65% |
| [Render/many_tags](#rendermany_tags) | 1,860,613 | -0.09% | -10.68% | 13 | 0% | -38.10% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 123,217 | -47.99% | -48.13% | 133 | -27.72% | -27.72% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 11,026,862 | 0% | -10.76% | 4,045 | 0% | -0.20% |
| [Load/plain_text](#loadplain_text) | 9,544 | -1.10% | -1.36% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,634 | -0.11% | -61.23% | 4 | 0% | -66.67% |
| [Load/strings](#loadstrings) | 154,811 | -47.05% | -46.96% | 142 | -36.61% | -36.61% |
| [Render/strings](#renderstrings) | 1,924,780 | +0.08% | -8.23% | 1,994 | 0% | -5.68% |
| [Load/substitute](#loadsubstitute) | 19,823 | -34.05% | -33.61% | 33 | -10.81% | -10.81% |
| [Render/substitute](#rendersubstitute) | 4,589 | -0.11% | -47.94% | 4 | 0% | -66.67% |

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
