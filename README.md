# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (102 so far, latest [45f79af](https://github.com/jinja2cpp/Jinja2Cpp/commit/45f79af7036336be4a27fd56e8617ae70998aa96) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 361,514 | +0.22% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,734 | +0.34% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 416,975 | +0.22% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,769 | +0.64% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 278,543 | +0.15% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,134 | +0.50% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,621 | +0.02% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,282,470 | +0.87% |  | 1,046 | 0% |  | 23,784 | +0.07% | -0.10% |
| [Load/dict_ops](#loaddict_ops) | 34,007 | +0.14% | -76.99% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 304,776 | +0.29% | -55.88% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,846 | +0.02% | -77.76% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 450,361 | +0.10% | -50.30% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,685 | +0.58% | -73.15% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,683 | +0.81% | -64.27% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,718 | -0.02% | -77.31% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,230 | +0.88% | -68.55% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,326 | +0.07% | -77.12% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,926 | +0.83% | -81.64% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,898 | -0.04% | -75.48% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,883 | +0.61% | -57.83% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,320 | +0.10% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,230,344 | +0.27% |  | 734 | 0% |  | 43,760 | +0.04% | +8.36% |
| [Load/inheritance](#loadinheritance) | 23,586 | -0.07% | -79.33% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 133,674 | +1.00% | -82.13% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 157,571 | +0.06% | -82.22% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,037 | +0.04% | -72.00% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,786 | +0.10% | -75.09% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 645,982 | +0.99% | -74.80% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,301,795 | +0.36% | -78.94% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,156 | +0.62% | -64.75% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,958 | +0.12% | -79.81% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,147,609 | +0.83% | -58.34% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,678 | +0.12% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,492,849 | +0.77% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,721 | -0.29% | -61.55% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,016 | +0.20% | -85.05% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,939 | +0.32% | -74.67% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 614,689 | +0.83% | -70.69% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,864 | -0.09% | -73.66% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,244 | +0.54% | -74.54% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 752 | +0.13% | 734 | -0.14% | 0 |  | 7,658 | -2.22% |
| Render/chat_llama | 1,046 | -0.38% | 414 | -0.48% | 0 |  | 3,499 | +3.64% |
| Load/chat_mistral | 1,083 | 0% | 1,003 | -0.59% | 0 |  | 8,647 | -2.04% |
| Render/chat_mistral | 1,785 | +0.06% | 422 | +0.72% | 0 |  | 8,734 | +2.80% |
| Load/chat_qwen | 526 | -0.38% | 608 | -0.82% | 0 |  | 4,810 | +0.48% |
| Render/chat_qwen | 935 | +0.65% | 313 | +1.29% | 0 |  | 3,033 | +3.66% |
| Load/config_file | 190 | +2.70% | 185 | -3.14% | 0 |  | 2,961 | -3.08% |
| Render/config_file | 5,862 | -1.45% | 1,376 | +0.22% | 0 |  | 37,788 | +0.99% |
| Load/dict_ops | 29 | -9.38% | 18 | 0% | 0 |  | 1,237 | -4.40% |
| Render/dict_ops | 700 | 0% | 435 | +0.93% | 0 |  | 858 | +0.82% |
| Load/expressions | 38 | +22.58% | 30 | +7.14% | 0 |  | 1,250 | -6.86% |
| Render/expressions | 6 | 0% | 3 | -25.00% | 0 |  | 265 | -11.67% |
| Load/filters | 127 | +0.79% | 64 | -4.48% | 0 |  | 1,866 | -2.10% |
| Render/filters | 19 | 0% | 5 | +25.00% | 0 |  | 811 | +2.79% |
| Load/for_filter_if | 7 | -36.36% | 6 | -14.29% | 0 |  | 1,045 | -5.52% |
| Render/for_filter_if | 1,711 | +0.06% | 72 | +5.88% | 0 |  | 159 | 0% |
| Load/for_loop_vars | 32 | +10.34% | 23 | -11.54% | 0 |  | 1,075 | -2.01% |
| Render/for_loop_vars | 6 | 0% | 5 | 0% | 0 |  | 133 | +3.91% |
| Load/for_range | 3 | 0% | 5 | +25.00% | 0 |  | 703 | -6.39% |
| Render/for_range | 3 | 0% | 3 | 0% | 0 |  | 82 | -45.70% |
| Load/html_autoescape | 488 | -0.20% | 502 | 0% | 0 |  | 4,152 | -3.37% |
| Render/html_autoescape | 3,231 | +1.38% | 1,070 | +2.10% | 0 |  | 33,465 | +5.84% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 628 | -0.48% |
| Render/inheritance | 568 | -0.35% | 130 | -1.52% | 0 |  | 785 | -2.97% |
| Load/large_static | 1,945 | 0% | 895 | -0.22% | 0 |  | 238 | -24.92% |
| Render/large_static | 703 | +0.29% | 574 | -0.35% | 0 |  | 3 | 0% |
| Load/macros | 64 | -7.25% | 55 | -9.84% | 0 |  | 1,625 | -3.73% |
| Render/macros | 711 | +0.28% | 244 | 0% | 0 |  | 2,417 | +364.81% |
| Load/many_tags | 28,358 | +2.07% | 21,645 | +0.57% | 0 |  | 254,130 | -0.12% |
| Render/many_tags | 6,038 | +0.03% | 874 | -0.23% | 0 |  | 145 | +83.54% |
| Load/mitsuhiko_table | 19 | -13.64% | 29 | +11.54% | 0 |  | 787 | -4.84% |
| Render/mitsuhiko_table | 9,022 | -0.03% | 6,614 | 0% | 0 |  | 288 | -9.15% |
| Load/mitsuhiko_table_wide | 85 | +6.25% | 84 | -2.33% | 0 |  | 968 | -3.01% |
| Render/mitsuhiko_table_wide | 9,179 | +0.02% | 22,926 | -0.01% | 0 |  | 310 | -5.20% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 11 | +120.00% |
| Render/plain_text | 0 |  | 1 |  | 0 |  | 6 | 0% |
| Load/strings | 172 | 0% | 115 | +0.88% | 0 |  | 2,702 | -2.77% |
| Render/strings | 476 | +0.85% | 618 | +0.32% | 0 |  | 1,426 | -0.49% |
| Load/substitute | 2 | 0% | 4 | -20.00% | 0 |  | 90 | -48.86% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |

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
