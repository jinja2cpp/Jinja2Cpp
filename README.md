# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (69 so far, latest [66c143e](https://github.com/jinja2cpp/Jinja2Cpp/commit/66c143edfc05ccfec7a5cfa7cae7a8abdc826c65) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 516,715 | -0.01% |  | 92 | 0% |  | 19,856 | 0% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 407,222 | +0.36% |  | 355 | 0% |  | 15,312 | 0% | +6.63% |
| [Load/chat_mistral](#loadchat_mistral) | 610,995 | -0.02% |  | 75 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 643,171 | +0.01% |  | 421 | 0% |  | 12,184 | 0% | -7.75% |
| [Load/chat_qwen](#loadchat_qwen) | 395,248 | -0.03% |  | 68 | 0% |  | 15,760 | 0% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 389,734 | +0.47% |  | 320 | 0% |  | 9,400 | 0% | +10.54% |
| [Load/config_file](#loadconfig_file) | 136,662 | -0.02% |  | 34 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,660,047 | -0.25% |  | 1,124 | 0% |  | 25,344 | 0% | +6.45% |
| [Load/dict_ops](#loaddict_ops) | 44,509 | +0.01% | -69.88% | 12 | 0% | -90.84% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 349,809 | -0.06% | -49.37% | 114 | 0% | -74.67% | 36,168 | 0% | +4.24% |
| [Load/expressions](#loadexpressions) | 61,200 | +0.01% | -70.31% | 18 | 0% | -87.67% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 514,897 | -0.46% | -43.18% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 91,128 | +0.12% | -66.79% | 32 | 0% | -85.96% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,331 | -0.91% | -40.98% | 37 | 0% | -50.00% | 2,568 | 0% | -47.72% |
| [Load/for_filter_if](#loadfor_filter_if) | 39,059 | -0.03% | -71.15% | 9 | 0% | -92.68% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 340,877 | +0.10% | -60.61% | 6 | 0% | -97.35% | 13,904 | 0% | +281.98% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 53,696 | -0.08% | -70.28% | 13 | 0% | -91.72% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,600 | -0.03% | -74.59% | 4 | 0% | -99.23% | 2,128 | 0% | -23.12% |
| [Load/for_range](#loadfor_range) | 18,390 | 0% | -67.56% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 45,959 | -0.58% | -50.16% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 269,572 | -0.04% |  | 61 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,546,746 | +0.01% |  | 938 | 0% |  | 45,824 | 0% | +13.47% |
| [Load/inheritance](#loadinheritance) | 32,893 | 0% | -71.18% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 149,743 | -0.09% | -79.98% | 34 | 0% | -94.53% | 7,176 | 0% | +19.28% |
| [Load/large_static](#loadlarge_static) | 220,869 | 0% | -75.07% | 28 | 0% | -93.64% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,622 | +0.70% | -71.22% | 3 | 0% | -83.33% | 37,112 | 0% | -0.32% |
| [Load/macros](#loadmacros) | 76,107 | +0.01% | -66.61% | 32 | 0% | -82.80% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 879,008 | +0.08% | -65.71% | 312 | 0% | -88.57% | 20,304 | 0% | +37.93% |
| [Load/many_tags](#loadmany_tags) | 12,365,086 | +0.07% | -72.00% | 962 | 0% | -97.19% | 445,680 | 0% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,141,573 | +0.24% | -45.20% | 15 | 0% | -28.57% | 82,520 | 0% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 70,981 | -0.01% | -70.12% | 26 | 0% | -85.87% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,453,756 | -1.58% | -47.77% | 35 | 0% | -99.14% | 373,872 | 0% | +7.74% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 89,422 | +0.01% |  | 57 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,804,207 | -1.78% |  | 53 | 0% |  | 1,407,264 | 0% | +1.90% |
| [Load/plain_text](#loadplain_text) | 4,015 | 0% | -58.51% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 987 | 0% | -85.48% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 94,526 | +0.12% | -67.61% | 33 | 0% | -85.27% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,054,892 | -0.07% | -49.71% | 979 | 0% | -53.69% | 18,384 | 0% | +2.54% |
| [Load/substitute](#loadsubstitute) | 10,129 | 0% | -66.07% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,686 | +0.04% | -69.53% | 3 | 0% | -75.00% | 600 | 0% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,079 | +0.37% | 1,165 | +0.69% | 0 |  | 8,204 | +3.26% |
| Render/chat_llama | 1,297 | -0.15% | 504 | -1.37% | 0 |  | 5,610 | +11.98% |
| Load/chat_mistral | 1,672 | -0.18% | 1,682 | -0.24% | 0 |  | 9,440 | +2.94% |
| Render/chat_mistral | 2,146 | -2.90% | 548 | -1.44% | 0 |  | 14,905 | +8.19% |
| Load/chat_qwen | 796 | -0.25% | 1,094 | +0.37% | 0 |  | 5,348 | +2.37% |
| Render/chat_qwen | 1,212 | -2.49% | 399 | +0.50% | 0 |  | 4,372 | -3.95% |
| Load/config_file | 314 | -0.63% | 434 | +0.46% | 0 |  | 3,246 | -0.28% |
| Render/config_file | 6,535 | -5.14% | 2,011 | -1.08% | 0 |  | 61,407 | +3.83% |
| Load/dict_ops | 62 | -3.12% | 51 | +8.51% | 0 |  | 1,461 | -0.27% |
| Render/dict_ops | 953 | -0.42% | 736 | +0.41% | 0 |  | 1,081 | -1.73% |
| Load/expressions | 83 | +1.22% | 103 | +7.29% | 0 |  | 1,404 | -2.64% |
| Render/expressions | 7 | -22.22% | 4 | +33.33% | 0 |  | 454 | +2.48% |
| Load/filters | 182 | -2.15% | 120 | +1.69% | 0 |  | 2,464 | +6.02% |
| Render/filters | 44 | -24.14% | 25 | -19.35% | 0 |  | 1,150 | +3.88% |
| Load/for_filter_if | 29 | +11.54% | 21 | +16.67% | 0 |  | 1,142 | 0% |
| Render/for_filter_if | 1,590 | +0.06% | 70 | +2.94% | 0 |  | 300 | +4.90% |
| Load/for_loop_vars | 43 | +7.50% | 44 | +15.79% | 0 |  | 1,301 | +2.20% |
| Render/for_loop_vars | 8 | -33.33% | 5 | -44.44% | 0 |  | 311 | +20.08% |
| Load/for_range | 16 | +60.00% | 12 | +20.00% | 0 |  | 895 | -0.11% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 214 | -12.30% |
| Load/html_autoescape | 618 | +0.82% | 783 | +0.51% | 0 |  | 4,692 | +2.31% |
| Render/html_autoescape | 5,057 | -11.02% | 1,634 | +4.74% | 0 |  | 48,578 | +1.42% |
| Load/inheritance | 10 | -28.57% | 12 | -7.69% | 0 |  | 860 | -0.12% |
| Render/inheritance | 581 | -0.68% | 127 | +3.25% | 0 |  | 874 | +0.23% |
| Load/large_static | 2,606 | +0.15% | 1,171 | 0% | 0 |  | 409 | +4.60% |
| Render/large_static | 710 | 0% | 578 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 177 | +1.72% | 226 | +2.26% | 0 |  | 1,862 | -0.80% |
| Render/macros | 778 | -0.64% | 231 | 0% | 0 |  | 6,194 | -39.36% |
| Load/many_tags | 36,410 | -0.88% | 28,695 | -0.41% | 0 |  | 289,258 | +6.01% |
| Render/many_tags | 6,888 | +0.03% | 1,457 | -0.48% | 0 |  | 2,508 | +41.78% |
| Load/mitsuhiko_table | 82 | +7.89% | 122 | +0.83% | 0 |  | 1,075 | -0.83% |
| Render/mitsuhiko_table | 9,299 | -0.15% | 6,505 | +0.05% | 0 |  | 513 | -96.74% |
| Load/mitsuhiko_table_wide | 166 | -0.60% | 253 | 0% | 0 |  | 1,642 | +2.18% |
| Render/mitsuhiko_table_wide | 9,399 | -0.16% | 22,995 | 0% | 0 |  | 4,857 | -67.77% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 13 | -23.53% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| Load/strings | 213 | -2.74% | 205 | -1.91% | 0 |  | 2,986 | -0.43% |
| Render/strings | 918 | -0.86% | 1,341 | -0.07% | 0 |  | 3,906 | +10.06% |
| Load/substitute | 7 | +40.00% | 10 | 0% | 0 |  | 316 | -2.17% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 4 | +33.33% |

## Charts

### Load/chat_llama

![Load/chat_llama](charts/Load_chat_llama.svg)

### Render/chat_llama

![Render/chat_llama](charts/Render_chat_llama.svg)

### Load/chat_mistral

![Load/chat_mistral](charts/Load_chat_mistral.svg)

### Render/chat_mistral

![Render/chat_mistral](charts/Render_chat_mistral.svg)

### Load/chat_qwen

![Load/chat_qwen](charts/Load_chat_qwen.svg)

### Render/chat_qwen

![Render/chat_qwen](charts/Render_chat_qwen.svg)

### Load/config_file

![Load/config_file](charts/Load_config_file.svg)

### Render/config_file

![Render/config_file](charts/Render_config_file.svg)

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

### Load/html_autoescape

![Load/html_autoescape](charts/Load_html_autoescape.svg)

### Render/html_autoescape

![Render/html_autoescape](charts/Render_html_autoescape.svg)

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

### Load/mitsuhiko_table_wide

![Load/mitsuhiko_table_wide](charts/Load_mitsuhiko_table_wide.svg)

### Render/mitsuhiko_table_wide

![Render/mitsuhiko_table_wide](charts/Render_mitsuhiko_table_wide.svg)

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
