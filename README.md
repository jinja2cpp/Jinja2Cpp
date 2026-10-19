# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (115 so far, latest [941c91c](https://github.com/jinja2cpp/Jinja2Cpp/commit/941c91c7c2908610de0900d6534eda472922c7b3) on 2026-10-19). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 353,908 | +1.21% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 315,062 | +1.40% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 405,520 | +1.41% |  | 50 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 522,963 | +0.71% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 272,291 | +1.22% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 299,810 | +1.45% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,595 | +1.30% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,287,110 | +0.10% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 34,070 | +1.16% | -76.95% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 307,755 | -0.02% | -55.45% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 46,297 | +1.52% | -77.54% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 454,433 | 0% | -49.85% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 74,026 | +0.97% | -73.03% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,005 | +0.11% | -65.03% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,827 | +1.20% | -77.23% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,489 | 0% | -68.63% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,396 | +1.22% | -77.09% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,830 | 0% | -81.84% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,951 | +1.01% | -75.39% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,368 | 0% | -57.31% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 200,828 | +1.28% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,236,515 | +0.19% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,685 | +1.02% | -79.25% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,682 | +0.01% | -81.72% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 156,983 | +0.52% | -82.28% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,077 | +0.02% | -71.95% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,911 | +1.31% | -75.03% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 650,020 | 0% | -74.64% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,326,427 | +1.29% | -78.88% | 956 | 0% | -97.21% | 421,328 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 734,999 | +0.29% | -64.71% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 48,090 | +1.20% | -79.76% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,197,681 | 0% | -57.93% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,128 | +0.17% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,912 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,747 | +0.16% | -61.28% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,051 | 0% | -84.53% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,374 | +1.33% | -74.52% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 624,286 | +0.48% | -70.24% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,866 | +0.49% | -73.65% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,289 | +0.44% | -74.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 715 | -3.12% | 733 | +0.14% | 0 |  | 7,249 | -0.47% |
| Render/chat_llama | 1,086 | +3.92% | 406 | -0.73% | 0 |  | 3,046 | -7.22% |
| Load/chat_mistral | 1,044 | +0.19% | 1,032 | +0.10% | 0 |  | 8,176 | -0.46% |
| Render/chat_mistral | 1,704 | +1.07% | 429 | -0.92% | 0 |  | 8,227 | -7.83% |
| Load/chat_qwen | 501 | -0.99% | 604 | +0.33% | 0 |  | 4,314 | -1.78% |
| Render/chat_qwen | 967 | +1.04% | 316 | -2.47% | 0 |  | 2,830 | +0.68% |
| Load/config_file | 162 | -6.36% | 174 | -3.33% | 0 |  | 2,852 | -1.49% |
| Render/config_file | 6,514 | +6.73% | 1,820 | +2.42% | 0 |  | 37,522 | -3.15% |
| Load/dict_ops | 27 | +3.85% | 17 | -10.53% | 0 |  | 1,248 | -3.70% |
| Render/dict_ops | 665 | -1.63% | 424 | -0.47% | 0 |  | 853 | +2.65% |
| Load/expressions | 34 | -2.86% | 17 | -32.00% | 0 |  | 1,250 | -4.14% |
| Render/expressions | 1 | -50.00% | 1 | 0% | 0 |  | 303 | +8.60% |
| Load/filters | 130 | +2.36% | 57 | -6.56% | 0 |  | 2,039 | +1.80% |
| Render/filters | 26 | 0% | 7 | -12.50% | 0 |  | 770 | +3.22% |
| Load/for_filter_if | 16 | +220.00% | 7 | +75.00% | 0 |  | 1,054 | -5.89% |
| Render/for_filter_if | 1,651 | -0.12% | 66 | +1.54% | 0 |  | 160 | 0% |
| Load/for_loop_vars | 21 | 0% | 12 | 0% | 0 |  | 1,081 | -2.17% |
| Render/for_loop_vars | 19 | +35.71% | 9 | +12.50% | 0 |  | 126 | +27.27% |
| Load/for_range | 6 | 0% | 5 | 0% | 0 |  | 692 | -2.54% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 110 | -1.79% |
| Load/html_autoescape | 399 | -4.09% | 500 | -0.40% | 0 |  | 3,994 | -6.31% |
| Render/html_autoescape | 3,854 | -7.13% | 1,198 | -16.46% | 0 |  | 29,660 | -8.63% |
| Load/inheritance | 12 | +33.33% | 6 | -14.29% | 0 |  | 616 | +5.48% |
| Render/inheritance | 575 | -0.17% | 110 | -3.51% | 0 |  | 808 | +1.00% |
| Load/large_static | 1,945 | +0.05% | 892 | 0% | 0 |  | 238 | -9.51% |
| Render/large_static | 707 | 0% | 572 | 0% | 0 |  | 4 | -63.64% |
| Load/macros | 75 | +5.63% | 58 | -4.92% | 0 |  | 1,586 | -4.57% |
| Render/macros | 1,227 | +57.11% | 251 | +0.40% | 0 |  | 548 | +7.45% |
| Load/many_tags | 27,576 | -0.65% | 21,162 | 0% | 0 |  | 252,414 | +0.82% |
| Render/many_tags | 6,020 | +0.08% | 849 | +0.59% | 0 |  | 109 | -38.07% |
| Load/mitsuhiko_table | 29 | +7.41% | 38 | -2.56% | 0 |  | 779 | -3.83% |
| Render/mitsuhiko_table | 9,080 | +0.15% | 6,498 | +0.02% | 0 |  | 290 | -4.61% |
| Load/mitsuhiko_table_wide | 74 | +7.25% | 67 | -1.47% | 0 |  | 983 | -3.72% |
| Render/mitsuhiko_table_wide | 9,145 | +0.11% | 22,958 | -0.01% | 0 |  | 321 | +9.93% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | -88.37% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 5 | +25.00% |
| Load/strings | 159 | -4.22% | 100 | +1.01% | 0 |  | 2,798 | +1.71% |
| Render/strings | 495 | +1.43% | 607 | -0.65% | 0 |  | 1,420 | 0% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 116 | -37.63% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |

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
