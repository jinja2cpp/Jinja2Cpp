# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (4 so far, latest [c465f9e](https://github.com/jinja2cpp/Jinja2Cpp/commit/c465f9ec6b94f3e069aad46eb2cb2912c0d033e9) on 2026-10-04). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 147,653 | 0% | -0.08% | 131 | 0% | 0% |
| [Render/dict_ops](#renderdict_ops) | 691,225 | 0% | +0.05% | 450 | 0% | 0% |
| [Load/expressions](#loadexpressions) | 206,037 | 0% | -0.05% | 146 | 0% | 0% |
| [Render/expressions](#renderexpressions) | 906,141 | 0% | 0% | 30 | 0% | 0% |
| [Load/filters](#loadfilters) | 274,351 | 0% | -0.03% | 228 | 0% | 0% |
| [Render/filters](#renderfilters) | 88,698 | 0% | +0.03% | 74 | 0% | 0% |
| [Load/for_filter_if](#loadfor_filter_if) | 135,241 | +0.01% | -0.11% | 123 | 0% | 0% |
| [Render/for_filter_if](#renderfor_filter_if) | 865,771 | 0% | +0.03% | 226 | 0% | 0% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 180,469 | 0% | -0.11% | 157 | 0% | 0% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 1,072,940 | 0% | +0.02% | 522 | 0% | 0% |
| [Load/for_range](#loadfor_range) | 56,614 | 0% | -0.12% | 56 | 0% | 0% |
| [Render/for_range](#renderfor_range) | 92,213 | 0% | 0% | 27 | 0% | 0% |
| [Load/inheritance](#loadinheritance) | 113,961 | 0% | -0.14% | 107 | 0% | 0% |
| [Render/inheritance](#renderinheritance) | 748,110 | 0% | +0.04% | 622 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 885,653 | 0% | -0.04% | 440 | 0% | 0% |
| [Render/large_static](#renderlarge_static) | 75,141 | 0% | +0.01% | 18 | 0% | 0% |
| [Load/macros](#loadmacros) | 227,824 | +0.01% | -0.06% | 186 | 0% | 0% |
| [Render/macros](#rendermacros) | 2,563,685 | 0% | +0.02% | 2,729 | 0% | 0% |
| [Load/many_tags](#loadmany_tags) | 44,132,451 | 0% | -0.07% | 34,249 | 0% | 0% |
| [Render/many_tags](#rendermany_tags) | 2,084,803 | 0% | +0.09% | 21 | 0% | 0% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 237,398 | 0% | -0.07% | 184 | 0% | 0% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 12,356,246 | 0% | 0% | 4,053 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 9,648 | 0% | -0.29% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 6,797 | 0% | +0.04% | 12 | 0% | 0% |
| [Load/strings](#loadstrings) | 291,680 | 0% | -0.06% | 224 | 0% | 0% |
| [Render/strings](#renderstrings) | 2,098,377 | 0% | +0.04% | 2,114 | 0% | 0% |
| [Load/substitute](#loadsubstitute) | 29,821 | 0% | -0.12% | 37 | 0% | 0% |
| [Render/substitute](#rendersubstitute) | 8,819 | 0% | +0.06% | 12 | 0% | 0% |

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
