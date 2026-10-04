# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (6 so far, latest [e68e327](https://github.com/jinja2cpp/Jinja2Cpp/commit/e68e327e044209e33cee03e81552a752b243a928) on 2026-10-04). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 147,536 | +0.04% | -0.16% | 131 | 0% | 0% |
| [Render/dict_ops](#renderdict_ops) | 518,052 | -1.08% | -25.01% | 331 | -3.50% | -26.44% |
| [Load/expressions](#loadexpressions) | 205,354 | -0.16% | -0.39% | 146 | 0% | 0% |
| [Render/expressions](#renderexpressions) | 819,443 | -1.04% | -9.57% | 18 | -40.00% | -40.00% |
| [Load/filters](#loadfilters) | 271,937 | +0.03% | -0.91% | 228 | 0% | 0% |
| [Render/filters](#renderfilters) | 82,762 | -4.74% | -6.66% | 66 | -10.81% | -10.81% |
| [Load/for_filter_if](#loadfor_filter_if) | 134,841 | 0% | -0.40% | 123 | 0% | 0% |
| [Render/for_filter_if](#renderfor_filter_if) | 705,244 | -0.76% | -18.52% | 218 | -3.54% | -3.54% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 180,352 | +0.22% | -0.17% | 157 | 0% | 0% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 874,234 | -0.45% | -18.50% | 514 | -1.53% | -1.53% |
| [Load/for_range](#loadfor_range) | 56,746 | +0.04% | +0.11% | 56 | 0% | 0% |
| [Render/for_range](#renderfor_range) | 76,756 | -7.28% | -16.76% | 15 | -44.44% | -44.44% |
| [Load/inheritance](#loadinheritance) | 113,931 | -0.03% | -0.16% | 107 | 0% | 0% |
| [Render/inheritance](#renderinheritance) | 678,609 | -2.44% | -9.26% | 572 | -8.04% | -8.04% |
| [Load/large_static](#loadlarge_static) | 881,517 | +0.05% | -0.50% | 440 | 0% | 0% |
| [Render/large_static](#renderlarge_static) | 69,300 | -5.67% | -7.77% | 10 | -44.44% | -44.44% |
| [Load/macros](#loadmacros) | 227,564 | +0.07% | -0.17% | 186 | 0% | 0% |
| [Render/macros](#rendermacros) | 1,461,959 | -40.18% | -42.96% | 422 | -84.54% | -84.54% |
| [Load/many_tags](#loadmany_tags) | 44,390,405 | +0.04% | +0.51% | 34,249 | 0% | 0% |
| [Render/many_tags](#rendermany_tags) | 1,862,349 | -0.40% | -10.59% | 13 | -38.10% | -38.10% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 236,892 | +0.02% | -0.28% | 184 | 0% | 0% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 11,026,903 | -0.24% | -10.76% | 4,045 | -0.20% | -0.20% |
| [Load/plain_text](#loadplain_text) | 9,650 | +0.02% | -0.27% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 2,637 | -61.21% | -61.19% | 4 | -66.67% | -66.67% |
| [Load/strings](#loadstrings) | 292,381 | +0.06% | +0.18% | 224 | 0% | 0% |
| [Render/strings](#renderstrings) | 1,923,615 | -2.40% | -8.29% | 1,994 | -5.68% | -5.68% |
| [Load/substitute](#loadsubstitute) | 30,059 | +0.06% | +0.68% | 37 | 0% | 0% |
| [Render/substitute](#rendersubstitute) | 4,594 | -47.55% | -47.88% | 4 | -66.67% | -66.67% |

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
