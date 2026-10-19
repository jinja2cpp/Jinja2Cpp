# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (116 so far, latest [9cf8586](https://github.com/jinja2cpp/Jinja2Cpp/commit/9cf8586185cb73d2c7dea01e96cd1588bc0a4c17) on 2026-10-19). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 357,214 | +0.93% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 315,062 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 408,840 | +0.82% |  | 50 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 522,963 | 0% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 274,646 | +0.86% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 299,810 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 102,520 | +0.91% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,287,110 | 0% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 34,457 | +1.14% | -76.68% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 307,755 | 0% | -55.45% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 46,617 | +0.69% | -77.39% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 454,433 | 0% | -49.85% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 74,515 | +0.66% | -72.85% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,005 | 0% | -65.03% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 31,201 | +1.21% | -76.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,489 | 0% | -68.63% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,914 | +1.25% | -76.80% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,830 | 0% | -81.84% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 14,142 | +1.37% | -75.05% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,368 | 0% | -57.31% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 202,654 | +0.91% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,236,515 | 0% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 24,072 | +1.63% | -78.91% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,682 | 0% | -81.72% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 159,240 | +1.44% | -82.03% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,077 | 0% | -71.95% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 57,418 | +0.89% | -74.81% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 650,020 | 0% | -74.64% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,430,235 | +1.11% | -78.65% | 956 | 0% | -97.21% | 421,328 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 734,999 | 0% | -64.71% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 48,603 | +1.07% | -79.54% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,197,681 | 0% | -57.93% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,589 | +0.81% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,912 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,814 | +1.79% | -60.58% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,051 | 0% | -84.53% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,859 | +0.65% | -74.35% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 624,286 | 0% | -70.24% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 8,019 | +1.95% | -73.14% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,289 | 0% | -74.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 754 | +5.45% | 742 | +1.23% | 0 |  | 7,492 | +3.35% |
| Render/chat_llama | 1,079 | -0.64% | 400 | -1.48% | 0 |  | 3,112 | +2.17% |
| Load/chat_mistral | 1,066 | +2.11% | 1,046 | +1.36% | 0 |  | 8,439 | +3.22% |
| Render/chat_mistral | 1,738 | +2.00% | 443 | +3.26% | 0 |  | 8,879 | +7.93% |
| Load/chat_qwen | 524 | +4.59% | 603 | -0.17% | 0 |  | 4,376 | +1.44% |
| Render/chat_qwen | 986 | +1.96% | 309 | -2.22% | 0 |  | 2,779 | -1.80% |
| Load/config_file | 168 | +3.70% | 179 | +2.87% | 0 |  | 2,876 | +0.84% |
| Render/config_file | 6,430 | -1.29% | 1,830 | +0.55% | 0 |  | 38,078 | +1.48% |
| Load/dict_ops | 21 | -22.22% | 14 | -17.65% | 0 |  | 1,311 | +5.05% |
| Render/dict_ops | 661 | -0.60% | 426 | +0.47% | 0 |  | 844 | -1.06% |
| Load/expressions | 43 | +26.47% | 21 | +23.53% | 0 |  | 1,326 | +6.08% |
| Render/expressions | 2 | +100.00% | 2 | +100.00% | 0 |  | 259 | -14.52% |
| Load/filters | 135 | +3.85% | 62 | +8.77% | 0 |  | 2,071 | +1.57% |
| Render/filters | 28 | +7.69% | 8 | +14.29% | 0 |  | 774 | +0.52% |
| Load/for_filter_if | 13 | -18.75% | 5 | -28.57% | 0 |  | 1,067 | +1.23% |
| Render/for_filter_if | 1,652 | +0.06% | 66 | 0% | 0 |  | 152 | -5.00% |
| Load/for_loop_vars | 21 | 0% | 15 | +25.00% | 0 |  | 1,085 | +0.37% |
| Render/for_loop_vars | 13 | -31.58% | 7 | -22.22% | 0 |  | 139 | +10.32% |
| Load/for_range | 7 | +16.67% | 5 | 0% | 0 |  | 716 | +3.47% |
| Render/for_range | 5 | +400.00% | 4 | +300.00% | 0 |  | 100 | -9.09% |
| Load/html_autoescape | 422 | +5.76% | 493 | -1.40% | 0 |  | 4,187 | +4.83% |
| Render/html_autoescape | 4,231 | +9.78% | 1,325 | +10.60% | 0 |  | 32,238 | +8.69% |
| Load/inheritance | 9 | -25.00% | 4 | -33.33% | 0 |  | 621 | +0.81% |
| Render/inheritance | 577 | +0.35% | 110 | 0% | 0 |  | 811 | +0.37% |
| Load/large_static | 1,947 | +0.10% | 890 | -0.22% | 0 |  | 345 | +44.96% |
| Render/large_static | 707 | 0% | 572 | 0% | 0 |  | 3 | -25.00% |
| Load/macros | 82 | +9.33% | 58 | 0% | 0 |  | 1,686 | +6.31% |
| Render/macros | 1,228 | +0.08% | 249 | -0.80% | 0 |  | 1,684 | +207.30% |
| Load/many_tags | 27,794 | +0.79% | 21,140 | -0.10% | 0 |  | 243,232 | -3.64% |
| Render/many_tags | 6,015 | -0.08% | 848 | -0.12% | 0 |  | 154 | +41.28% |
| Load/mitsuhiko_table | 31 | +6.90% | 37 | -2.63% | 0 |  | 865 | +11.04% |
| Render/mitsuhiko_table | 9,073 | -0.08% | 6,502 | +0.06% | 0 |  | 290 | 0% |
| Load/mitsuhiko_table_wide | 77 | +4.05% | 71 | +5.97% | 0 |  | 1,044 | +6.21% |
| Render/mitsuhiko_table_wide | 9,139 | -0.07% | 22,960 | +0.01% | 0 |  | 324 | +0.93% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 4 | -20.00% |
| Load/strings | 170 | +6.92% | 98 | -2.00% | 0 |  | 2,805 | +0.25% |
| Render/strings | 491 | -0.81% | 607 | 0% | 0 |  | 1,447 | +1.90% |
| Load/substitute | 1 | -50.00% | 3 | -25.00% | 0 |  | 159 | +37.07% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |

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
