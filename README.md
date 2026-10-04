# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (5 so far, latest [a774747](https://github.com/jinja2cpp/Jinja2Cpp/commit/a77474754f3275c2868b00d81478c886701a53ec) on 2026-10-04). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 147,472 | -0.12% | -0.21% | 131 | 0% | 0% |
| [Render/dict_ops](#renderdict_ops) | 523,704 | -24.24% | -24.19% | 343 | -23.78% | -23.78% |
| [Load/expressions](#loadexpressions) | 205,674 | -0.18% | -0.23% | 146 | 0% | 0% |
| [Render/expressions](#renderexpressions) | 828,062 | -8.62% | -8.62% | 30 | 0% | 0% |
| [Load/filters](#loadfilters) | 271,843 | -0.91% | -0.95% | 228 | 0% | 0% |
| [Render/filters](#renderfilters) | 86,879 | -2.05% | -2.02% | 74 | 0% | 0% |
| [Load/for_filter_if](#loadfor_filter_if) | 134,843 | -0.29% | -0.40% | 123 | 0% | 0% |
| [Render/for_filter_if](#renderfor_filter_if) | 710,680 | -17.91% | -17.89% | 226 | 0% | 0% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 179,959 | -0.28% | -0.39% | 157 | 0% | 0% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 878,229 | -18.15% | -18.13% | 522 | 0% | 0% |
| [Load/for_range](#loadfor_range) | 56,724 | +0.19% | +0.07% | 56 | 0% | 0% |
| [Render/for_range](#renderfor_range) | 82,779 | -10.23% | -10.23% | 27 | 0% | 0% |
| [Load/inheritance](#loadinheritance) | 113,961 | 0% | -0.14% | 107 | 0% | 0% |
| [Render/inheritance](#renderinheritance) | 695,550 | -7.03% | -6.99% | 622 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 881,111 | -0.51% | -0.55% | 440 | 0% | 0% |
| [Render/large_static](#renderlarge_static) | 73,464 | -2.23% | -2.23% | 18 | 0% | 0% |
| [Load/macros](#loadmacros) | 227,396 | -0.19% | -0.24% | 186 | 0% | 0% |
| [Render/macros](#rendermacros) | 2,444,033 | -4.67% | -4.64% | 2,729 | 0% | 0% |
| [Load/many_tags](#loadmany_tags) | 44,372,099 | +0.54% | +0.47% | 34,249 | 0% | 0% |
| [Render/many_tags](#rendermany_tags) | 1,869,790 | -10.31% | -10.24% | 21 | 0% | 0% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 236,852 | -0.23% | -0.30% | 184 | 0% | 0% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 11,053,255 | -10.55% | -10.55% | 4,053 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 9,648 | 0% | -0.29% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 6,797 | 0% | +0.04% | 12 | 0% | 0% |
| [Load/strings](#loadstrings) | 292,211 | +0.18% | +0.12% | 224 | 0% | 0% |
| [Render/strings](#renderstrings) | 1,971,007 | -6.07% | -6.03% | 2,114 | 0% | 0% |
| [Load/substitute](#loadsubstitute) | 30,041 | +0.74% | +0.62% | 37 | 0% | 0% |
| [Render/substitute](#rendersubstitute) | 8,758 | -0.69% | -0.64% | 12 | 0% | 0% |

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
