# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (117 so far, latest [8030f97](https://github.com/jinja2cpp/Jinja2Cpp/commit/8030f97164cc06b6a5c0c1adcba63ca2f28d9743) on 2026-10-19). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 357,209 | 0% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 308,070 | -2.22% |  | 251 | -4.20% |  | 11,136 | +8.84% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 408,874 | +0.01% |  | 50 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 515,047 | -1.51% |  | 428 | -4.46% |  | 9,520 | +4.94% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 274,685 | +0.01% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 293,384 | -2.14% |  | 238 | -4.42% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 102,520 | 0% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,257,773 | -2.28% |  | 908 | -13.19% |  | 23,752 | -0.20% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 34,464 | +0.02% | -76.68% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 298,788 | -2.91% | -56.75% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 46,617 | 0% | -77.39% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 454,304 | -0.03% | -49.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 74,515 | 0% | -72.85% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,253 | +0.80% | -64.75% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 31,187 | -0.04% | -76.96% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,350 | -0.05% | -68.65% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,914 | 0% | -76.80% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,630 | -0.10% | -81.86% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 14,142 | 0% | -75.05% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,239 | -0.33% | -57.45% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 202,593 | -0.03% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,232,887 | -0.29% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 24,072 | 0% | -78.91% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,682 | 0% | -81.72% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 159,240 | 0% | -82.03% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,077 | 0% | -71.95% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 57,411 | -0.01% | -74.81% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 648,620 | -0.22% | -74.69% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,430,235 | 0% | -78.65% | 956 | 0% | -97.21% | 421,328 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 734,699 | -0.04% | -64.73% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 48,603 | 0% | -79.54% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,197,687 | 0% | -57.93% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,583 | -0.01% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,918 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,814 | 0% | -60.58% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,051 | 0% | -84.53% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,822 | -0.05% | -74.36% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 633,471 | +1.47% | -69.80% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 8,019 | 0% | -73.14% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,289 | 0% | -74.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 728 | -3.45% | 728 | -1.89% | 0 |  | 7,391 | -1.35% |
| Render/chat_llama | 1,110 | +2.87% | 485 | +21.25% | 0 |  | 3,302 | +6.11% |
| Load/chat_mistral | 1,043 | -2.16% | 1,039 | -0.67% | 0 |  | 7,978 | -5.46% |
| Render/chat_mistral | 1,797 | +3.39% | 509 | +14.90% | 0 |  | 8,789 | -1.01% |
| Load/chat_qwen | 502 | -4.20% | 601 | -0.33% | 0 |  | 4,167 | -4.78% |
| Render/chat_qwen | 1,089 | +10.45% | 417 | +34.95% | 0 |  | 3,055 | +9.93% |
| Load/config_file | 152 | -9.52% | 177 | -1.12% | 0 |  | 2,804 | -2.50% |
| Render/config_file | 5,886 | -8.46% | 1,699 | -7.16% | 0 |  | 39,688 | +4.23% |
| Load/dict_ops | 21 | 0% | 12 | -14.29% | 0 |  | 1,265 | -3.51% |
| Render/dict_ops | 754 | +14.07% | 428 | +0.47% | 0 |  | 848 | +0.47% |
| Load/expressions | 19 | -55.81% | 11 | -47.62% | 0 |  | 1,257 | -5.20% |
| Render/expressions | 5 | +150.00% | 4 | +100.00% | 0 |  | 250 | -3.47% |
| Load/filters | 127 | -5.93% | 54 | -12.90% | 0 |  | 2,093 | +1.06% |
| Render/filters | 30 | +7.14% | 8 | 0% | 0 |  | 758 | -2.07% |
| Load/for_filter_if | 18 | +38.46% | 8 | +60.00% | 0 |  | 1,036 | -2.91% |
| Render/for_filter_if | 1,652 | 0% | 63 | -4.55% | 0 |  | 207 | +36.18% |
| Load/for_loop_vars | 18 | -14.29% | 12 | -20.00% | 0 |  | 1,024 | -5.62% |
| Render/for_loop_vars | 12 | -7.69% | 9 | +28.57% | 0 |  | 118 | -15.11% |
| Load/for_range | 3 | -57.14% | 4 | -20.00% | 0 |  | 701 | -2.09% |
| Render/for_range | 1 | -80.00% | 0 | -100.00% | 0 |  | 100 | 0% |
| Load/html_autoescape | 421 | -0.24% | 499 | +1.22% | 0 |  | 4,059 | -3.06% |
| Render/html_autoescape | 3,711 | -12.29% | 1,206 | -8.98% | 0 |  | 30,718 | -4.71% |
| Load/inheritance | 3 | -66.67% | 3 | -25.00% | 0 |  | 608 | -2.09% |
| Render/inheritance | 580 | +0.52% | 106 | -3.64% | 0 |  | 791 | -2.47% |
| Load/large_static | 1,948 | +0.05% | 890 | 0% | 0 |  | 249 | -27.83% |
| Render/large_static | 708 | +0.14% | 572 | 0% | 0 |  | 13 | +333.33% |
| Load/macros | 68 | -17.07% | 48 | -17.24% | 0 |  | 1,607 | -4.69% |
| Render/macros | 716 | -41.69% | 250 | +0.40% | 0 |  | 2,291 | +36.05% |
| Load/many_tags | 28,116 | +1.16% | 21,223 | +0.39% | 0 |  | 255,601 | +5.09% |
| Render/many_tags | 6,022 | +0.12% | 842 | -0.71% | 0 |  | 152 | -1.30% |
| Load/mitsuhiko_table | 26 | -16.13% | 42 | +13.51% | 0 |  | 801 | -7.40% |
| Render/mitsuhiko_table | 9,060 | -0.14% | 6,499 | -0.05% | 0 |  | 322 | +11.03% |
| Load/mitsuhiko_table_wide | 67 | -12.99% | 66 | -7.04% | 0 |  | 899 | -13.89% |
| Render/mitsuhiko_table_wide | 9,162 | +0.25% | 22,959 | 0% | 0 |  | 333 | +2.78% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 7 | +75.00% |
| Load/strings | 158 | -7.06% | 95 | -3.06% | 0 |  | 2,680 | -4.46% |
| Render/strings | 505 | +2.85% | 606 | -0.16% | 0 |  | 1,488 | +2.83% |
| Load/substitute | 1 | 0% | 3 | 0% | 0 |  | 134 | -15.72% |
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
