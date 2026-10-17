# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (99 so far, latest [364e749](https://github.com/jinja2cpp/Jinja2Cpp/commit/364e7496e699c26cf466da2faa186e830834fcba) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 362,684 | +0.14% |  | 64 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 314,072 | +1.30% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 420,290 | +0.05% |  | 53 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 522,950 | +0.55% |  | 448 | -0.44% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 279,060 | +0.12% |  | 46 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 299,821 | +1.34% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 102,883 | +0.01% |  | 30 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,264,066 | +0.17% |  | 1,046 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 34,172 | +0.01% | -76.88% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 303,758 | -0.30% | -56.03% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,778 | -0.06% | -77.79% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 449,911 | -2.78% | -50.35% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,516 | +0.03% | -73.21% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,415 | +0.82% | -64.57% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,899 | +0.01% | -77.18% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 269,837 | -0.61% | -68.82% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,423 | +0.01% | -77.07% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 195,258 | +0.01% | -81.80% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 14,051 | +0.01% | -75.21% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,633 | +0.02% | -58.10% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 202,658 | +0.05% |  | 52 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,226,735 | +0.17% |  | 734 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 24,017 | 0% | -78.95% | 8 | 0% | -92.52% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,200 | +0.09% | -82.46% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 158,850 | 0% | -82.07% | 21 | 0% | -95.23% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,015 | 0% | -72.03% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 57,213 | +0.01% | -74.90% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 638,828 | -0.75% | -75.08% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,385,552 | +0.03% | -78.75% | 964 | 0% | -97.19% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 729,567 | +0.17% | -64.98% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 48,217 | -0.01% | -79.70% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,105,465 | -0.02% | -58.68% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,350 | +0.20% |  | 36 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,450,719 | -0.02% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,841 | -0.03% | -60.31% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | 0% | -85.29% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,544 | +0.03% | -74.46% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 608,317 | +1.05% | -71.00% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,998 | -0.01% | -73.21% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,219 | 0% | -74.83% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 770 | -0.26% | 756 | +0.53% | 0 |  | 7,202 | -2.31% |
| Render/chat_llama | 1,081 | +1.41% | 424 | +2.91% | 0 |  | 3,174 | -7.92% |
| Load/chat_mistral | 1,082 | -0.82% | 1,019 | -0.29% | 0 |  | 8,018 | -4.22% |
| Render/chat_mistral | 1,815 | +1.74% | 433 | +6.39% | 0 |  | 8,733 | -11.22% |
| Load/chat_qwen | 531 | +1.14% | 622 | +0.32% | 0 |  | 4,193 | -4.64% |
| Render/chat_qwen | 939 | +0.54% | 311 | +4.01% | 0 |  | 2,857 | +1.28% |
| Load/config_file | 184 | +0.55% | 181 | -2.16% | 0 |  | 2,978 | -3.56% |
| Render/config_file | 6,108 | +1.92% | 1,331 | +1.60% | 0 |  | 37,852 | -2.24% |
| Load/dict_ops | 29 | +3.57% | 19 | +11.76% | 0 |  | 1,277 | -2.67% |
| Render/dict_ops | 698 | -0.29% | 432 | -0.46% | 0 |  | 825 | -4.40% |
| Load/expressions | 37 | +19.35% | 26 | +30.00% | 0 |  | 1,223 | -1.37% |
| Render/expressions | 5 | -16.67% | 3 | 0% | 0 |  | 301 | +0.67% |
| Load/filters | 121 | -0.82% | 58 | -4.92% | 0 |  | 2,146 | +3.57% |
| Render/filters | 15 | +25.00% | 7 | +40.00% | 0 |  | 818 | +4.07% |
| Load/for_filter_if | 8 | -46.67% | 6 | -25.00% | 0 |  | 1,025 | -6.82% |
| Render/for_filter_if | 1,712 | 0% | 67 | +6.35% | 0 |  | 178 | +49.58% |
| Load/for_loop_vars | 33 | +6.45% | 22 | +15.79% | 0 |  | 1,018 | -7.62% |
| Render/for_loop_vars | 6 | 0% | 3 | 0% | 0 |  | 101 | -12.17% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 733 | -1.35% |
| Render/for_range | 1 | -66.67% | 1 | -66.67% | 0 |  | 93 | +3.33% |
| Load/html_autoescape | 501 | -0.40% | 512 | +0.20% | 0 |  | 3,968 | -6.59% |
| Render/html_autoescape | 2,789 | -3.06% | 1,101 | -3.76% | 0 |  | 31,604 | -6.23% |
| Load/inheritance | 2 | 0% | 2 | 0% | 0 |  | 658 | +2.02% |
| Render/inheritance | 570 | +1.42% | 134 | +2.29% | 0 |  | 746 | -1.45% |
| Load/large_static | 1,954 | -0.05% | 905 | 0% | 0 |  | 295 | +1.03% |
| Render/large_static | 700 | 0% | 576 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 69 | 0% | 57 | -1.72% | 0 |  | 1,662 | -3.20% |
| Render/macros | 715 | +0.14% | 240 | +1.27% | 0 |  | 2,231 | +46.39% |
| Load/many_tags | 28,661 | +0.61% | 22,228 | +0.28% | 0 |  | 256,804 | -1.93% |
| Render/many_tags | 6,017 | +0.03% | 869 | +0.70% | 0 |  | 123 | -3.15% |
| Load/mitsuhiko_table | 21 | +10.53% | 27 | +12.50% | 0 |  | 808 | -4.04% |
| Render/mitsuhiko_table | 9,039 | +0.03% | 6,609 | +0.03% | 0 |  | 287 | -1.37% |
| Load/mitsuhiko_table_wide | 79 | -1.25% | 72 | -2.70% | 0 |  | 1,078 | +10.79% |
| Render/mitsuhiko_table_wide | 9,194 | +0.02% | 22,918 | 0% | 0 |  | 283 | -2.08% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 14 | +180.00% |
| Render/plain_text | 0 |  | 1 |  | 0 |  | 7 | +16.67% |
| Load/strings | 151 | +0.67% | 94 | -3.09% | 0 |  | 2,905 | +2.04% |
| Render/strings | 482 | +1.69% | 616 | +0.33% | 0 |  | 1,451 | +0.48% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 161 | +4.55% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 6 | +100.00% |

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
