# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (112 so far, latest [523f7ab](https://github.com/jinja2cpp/Jinja2Cpp/commit/523f7ab4d335103c62a2df92610ec3c45a2130c5) on 2026-10-18). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,498 | 0% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,888 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 400,319 | 0% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,448 | +0.02% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 268,821 | 0% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,135 | +0.01% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,603 | -0.03% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,291,125 | +0.01% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 33,712 | -0.01% | -77.19% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 307,555 | +0.01% | -55.48% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,606 | -0.01% | -77.88% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 451,081 | 0% | -50.22% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,319 | -0.03% | -73.28% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,747 | +0.03% | -64.20% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,411 | -0.03% | -77.54% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,680 | +0.01% | -68.49% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,905 | +0.03% | -77.36% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,459 | +0.01% | -81.69% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,790 | 0% | -75.67% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,303 | +0.05% | -57.38% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,625 | -0.02% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,242,690 | +0.04% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,301 | 0% | -79.58% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,490 | +0.01% | -81.75% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 156,170 | 0% | -82.37% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,065 | +0.07% | -71.96% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,175 | 0% | -75.36% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 649,415 | 0% | -74.66% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,211,197 | -0.01% | -79.14% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,484 | 0% | -64.74% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,556 | 0% | -79.98% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,185,624 | 0% | -58.03% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,454 | +0.06% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,530,868 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,720 | 0% | -61.56% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,044 | +1.36% | -84.64% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,484 | -0.01% | -74.82% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 624,503 | +0.14% | -70.23% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,807 | 0% | -73.85% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,272 | +0.62% | -74.23% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 713 | -1.66% | 723 | +0.14% | 0 |  | 7,368 | -2.35% |
| Render/chat_llama | 1,071 | +0.94% | 424 | +0.71% | 0 |  | 3,921 | +6.75% |
| Load/chat_mistral | 1,064 | +0.28% | 1,022 | +0.89% | 0 |  | 7,924 | -6.71% |
| Render/chat_mistral | 1,789 | +2.52% | 417 | -0.71% | 0 |  | 9,760 | +7.17% |
| Load/chat_qwen | 495 | -0.40% | 596 | -0.67% | 0 |  | 4,146 | -10.32% |
| Render/chat_qwen | 1,022 | +8.49% | 308 | +4.05% | 0 |  | 3,061 | +6.03% |
| Load/config_file | 172 | +6.83% | 178 | +5.33% | 0 |  | 2,859 | -4.51% |
| Render/config_file | 6,341 | -1.80% | 1,827 | -5.78% | 0 |  | 38,092 | +0.56% |
| Load/dict_ops | 35 | +6.06% | 18 | -18.18% | 0 |  | 1,286 | 0% |
| Render/dict_ops | 688 | +2.38% | 428 | -1.83% | 0 |  | 860 | +3.12% |
| Load/expressions | 38 | +26.67% | 22 | +15.79% | 0 |  | 1,289 | -0.92% |
| Render/expressions | 3 | -57.14% | 1 | -80.00% | 0 |  | 306 | -3.16% |
| Load/filters | 111 | +4.72% | 61 | 0% | 0 |  | 2,037 | -10.03% |
| Render/filters | 29 | +81.25% | 8 | +100.00% | 0 |  | 815 | +2.39% |
| Load/for_filter_if | 15 | +25.00% | 9 | +28.57% | 0 |  | 1,048 | -3.05% |
| Render/for_filter_if | 1,711 | -0.23% | 65 | -7.14% | 0 |  | 151 | +12.69% |
| Load/for_loop_vars | 34 | +21.43% | 24 | 0% | 0 |  | 1,017 | -8.38% |
| Render/for_loop_vars | 7 | -12.50% | 5 | -16.67% | 0 |  | 123 | -8.21% |
| Load/for_range | 3 | 0% | 5 | +25.00% | 0 |  | 724 | +3.43% |
| Render/for_range | 1 | -85.71% | 1 | -75.00% | 0 |  | 147 | 0% |
| Load/html_autoescape | 477 | +3.25% | 505 | +0.60% | 0 |  | 3,868 | -11.08% |
| Render/html_autoescape | 3,952 | -8.33% | 1,261 | -6.73% | 0 |  | 33,104 | +0.63% |
| Load/inheritance | 3 | +50.00% | 4 | +33.33% | 0 |  | 626 | -2.95% |
| Render/inheritance | 568 | -0.53% | 125 | 0% | 0 |  | 817 | -3.43% |
| Load/large_static | 1,946 | -0.10% | 893 | 0% | 0 |  | 295 | +1.37% |
| Render/large_static | 704 | 0% | 575 | +0.17% | 0 |  | 4 | 0% |
| Load/macros | 72 | 0% | 58 | +5.45% | 0 |  | 1,609 | -2.01% |
| Render/macros | 726 | -0.41% | 248 | -1.20% | 0 |  | 575 | -62.34% |
| Load/many_tags | 27,523 | -0.27% | 21,436 | -0.02% | 0 |  | 252,053 | -1.80% |
| Render/many_tags | 6,043 | +0.12% | 874 | -0.23% | 0 |  | 133 | +5.56% |
| Load/mitsuhiko_table | 26 | -10.34% | 37 | +5.71% | 0 |  | 803 | -6.30% |
| Render/mitsuhiko_table | 9,026 | -0.08% | 6,614 | 0% | 0 |  | 332 | +4.40% |
| Load/mitsuhiko_table_wide | 89 | +28.99% | 79 | +12.86% | 0 |  | 957 | +2.13% |
| Render/mitsuhiko_table_wide | 9,155 | -0.78% | 22,923 | -0.06% | 0 |  | 344 | +21.99% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | -64.29% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 7 | 0% |
| Load/strings | 155 | -3.73% | 112 | +1.82% | 0 |  | 2,808 | -0.85% |
| Render/strings | 477 | +1.27% | 617 | +0.49% | 0 |  | 1,473 | +1.45% |
| Load/substitute | 3 | +50.00% | 4 | 0% | 0 |  | 140 | -25.13% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 3 | -25.00% |

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
