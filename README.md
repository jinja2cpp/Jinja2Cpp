# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (2 so far, latest [5a32857](https://github.com/jinja2cpp/Jinja2Cpp/commit/5a32857c9bb3b466460c86d979a8a402f3bd1fca) on 2026-10-04). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|
| [Load/dict_ops](#loaddict_ops) | 147,777 | 0% | 0% | 131 | 0% | 0% |
| [Render/dict_ops](#renderdict_ops) | 690,846 | 0% | 0% | 450 | 0% | 0% |
| [Load/expressions](#loadexpressions) | 206,148 | 0% | 0% | 146 | 0% | 0% |
| [Render/expressions](#renderexpressions) | 906,138 | 0% | 0% | 30 | 0% | 0% |
| [Load/filters](#loadfilters) | 274,439 | 0% | 0% | 228 | 0% | 0% |
| [Render/filters](#renderfilters) | 88,669 | 0% | 0% | 74 | 0% | 0% |
| [Load/for_filter_if](#loadfor_filter_if) | 135,388 | 0% | 0% | 123 | 0% | 0% |
| [Render/for_filter_if](#renderfor_filter_if) | 865,490 | 0% | 0% | 226 | 0% | 0% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 180,659 | 0% | 0% | 157 | 0% | 0% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 1,072,737 | 0% | 0% | 522 | 0% | 0% |
| [Load/for_range](#loadfor_range) | 56,683 | 0% | 0% | 56 | 0% | 0% |
| [Render/for_range](#renderfor_range) | 92,210 | 0% | 0% | 27 | 0% | 0% |
| [Load/inheritance](#loadinheritance) | 114,118 | 0% | 0% | 107 | 0% | 0% |
| [Render/inheritance](#renderinheritance) | 747,848 | 0% | 0% | 622 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 885,981 | 0% | 0% | 440 | 0% | 0% |
| [Render/large_static](#renderlarge_static) | 75,136 | 0% | 0% | 18 | 0% | 0% |
| [Load/macros](#loadmacros) | 227,951 | 0% | 0% | 186 | 0% | 0% |
| [Render/macros](#rendermacros) | 2,563,082 | 0% | 0% | 2,729 | 0% | 0% |
| [Load/many_tags](#loadmany_tags) | 44,164,633 | 0% | 0% | 34,249 | 0% | 0% |
| [Render/many_tags](#rendermany_tags) | 2,083,000 | 0% | 0% | 21 | 0% | 0% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 237,561 | 0% | 0% | 184 | 0% | 0% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 12,356,241 | 0% | 0% | 4,053 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 9,676 | 0% | 0% | 18 | 0% | 0% |
| [Render/plain_text](#renderplain_text) | 6,794 | 0% | 0% | 12 | 0% | 0% |
| [Load/strings](#loadstrings) | 291,849 | 0% | 0% | 224 | 0% | 0% |
| [Render/strings](#renderstrings) | 2,097,510 | 0% | 0% | 2,114 | 0% | 0% |
| [Load/substitute](#loadsubstitute) | 29,857 | 0% | 0% | 37 | 0% | 0% |
| [Render/substitute](#rendersubstitute) | 8,814 | 0% | 0% | 12 | 0% | 0% |

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
