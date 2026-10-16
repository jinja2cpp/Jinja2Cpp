# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (93 so far, latest [e28e3d8](https://github.com/jinja2cpp/Jinja2Cpp/commit/e28e3d8154025abe78073c32f34c0108954ef5fa) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 356,048 | +0.04% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 306,277 | -0.34% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,306 | -0.02% |  | 52 | 0% |  | 20,664 | 0% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 513,833 | -0.36% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 275,863 | -0.06% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 292,854 | -0.01% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,570 | +0.03% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,251,245 | +0.07% |  | 1,046 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 33,607 | -0.09% | -77.26% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 301,374 | +0.05% | -56.38% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,244 | -0.02% | -78.05% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,709 | 0% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,926 | +0.01% | -73.43% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 30,920 | -0.16% | -65.13% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,406 | +0.11% | -77.54% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 269,784 | +0.09% | -68.83% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,905 | +0.14% | -77.36% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,292 | +0.03% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,752 | +0.01% | -75.74% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,631 | +0.01% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 199,333 | +0.22% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,221,552 | +0.05% |  | 734 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 23,283 | +0.01% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,169 | -0.10% | -82.46% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 156,454 | +0.04% | -82.34% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,019 | -0.13% | -72.03% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,081 | +0.06% | -75.40% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 641,051 | -0.21% | -74.99% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,195,024 | 0% | -79.18% | 945 | 0% | -97.24% | 421,440 | +0.03% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 729,665 | 0% | -64.97% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,466 | +0.01% | -80.02% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,106,204 | -0.02% | -58.68% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,795 | +0.45% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,451,528 | -0.02% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,697 | +0.05% | -61.79% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,004 | +0.40% | -85.22% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,101 | +0.02% | -74.95% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 600,294 | +0.11% | -71.38% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,792 | +0.03% | -73.90% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,223 | +0.18% | -74.78% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 707 | +1.29% | 733 | +5.77% | 0 |  | 7,042 | +1.95% |
| Render/chat_llama | 1,075 | -2.45% | 440 | +1.85% | 0 |  | 3,630 | +13.97% |
| Load/chat_mistral | 1,013 | -0.39% | 1,006 | +2.76% | 0 |  | 8,051 | +7.79% |
| Render/chat_mistral | 1,723 | -2.21% | 500 | +4.60% | 0 |  | 9,142 | +5.58% |
| Load/chat_qwen | 514 | -1.15% | 605 | +1.34% | 0 |  | 4,650 | +10.85% |
| Render/chat_qwen | 944 | -8.70% | 353 | -4.08% | 0 |  | 2,917 | +8.92% |
| Load/config_file | 171 | -9.04% | 162 | -6.90% | 0 |  | 2,971 | +6.22% |
| Render/config_file | 5,397 | +3.07% | 1,420 | +11.64% | 0 |  | 36,221 | +0.01% |
| Load/dict_ops | 23 | -14.81% | 11 | -45.00% | 0 |  | 1,250 | +1.05% |
| Render/dict_ops | 671 | +2.29% | 430 | +0.70% | 0 |  | 828 | -0.60% |
| Load/expressions | 33 | -17.50% | 27 | -20.59% | 0 |  | 1,159 | -2.28% |
| Render/expressions | 2 | -81.82% | 2 | -66.67% | 0 |  | 251 | -17.70% |
| Load/filters | 123 | -0.81% | 59 | -19.18% | 0 |  | 2,095 | -1.78% |
| Render/filters | 18 | +20.00% | 7 | +75.00% | 0 |  | 792 | -1.12% |
| Load/for_filter_if | 13 | +44.44% | 8 | -11.11% | 0 |  | 1,094 | +8.96% |
| Render/for_filter_if | 1,711 | -0.64% | 65 | -1.52% | 0 |  | 167 | +10.60% |
| Load/for_loop_vars | 30 | +11.11% | 23 | +53.33% | 0 |  | 1,136 | +7.37% |
| Render/for_loop_vars | 3 | -40.00% | 1 | 0% | 0 |  | 108 | -5.26% |
| Load/for_range | 2 | -60.00% | 4 | -50.00% | 0 |  | 726 | +3.12% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 66 | -50.00% |
| Load/html_autoescape | 464 | +4.98% | 494 | -0.80% | 0 |  | 4,527 | +12.14% |
| Render/html_autoescape | 2,537 | -22.13% | 887 | -24.45% | 0 |  | 32,501 | -1.53% |
| Load/inheritance | 3 | 0% | 3 | +50.00% | 0 |  | 576 | -4.95% |
| Render/inheritance | 572 | -2.89% | 128 | 0% | 0 |  | 772 | -2.03% |
| Load/large_static | 1,947 | -0.05% | 886 | 0% | 0 |  | 248 | -13.89% |
| Render/large_static | 702 | -0.57% | 576 | +0.35% | 0 |  | 4 | +33.33% |
| Load/macros | 83 | +27.69% | 76 | +61.70% | 0 |  | 1,634 | +4.08% |
| Render/macros | 711 | -5.70% | 241 | +0.42% | 0 |  | 495 | +0.20% |
| Load/many_tags | 27,025 | +1.02% | 20,562 | -0.26% | 0 |  | 252,657 | +4.69% |
| Render/many_tags | 6,052 | +0.03% | 858 | -1.49% | 0 |  | 142 | +12.70% |
| Load/mitsuhiko_table | 22 | 0% | 38 | +90.00% | 0 |  | 752 | -1.18% |
| Render/mitsuhiko_table | 9,022 | -2.87% | 6,608 | -0.20% | 0 |  | 266 | -10.74% |
| Load/mitsuhiko_table_wide | 99 | +16.47% | 103 | -8.85% | 0 |  | 1,243 | -1.43% |
| Render/mitsuhiko_table_wide | 9,154 | -2.48% | 23,047 | +0.58% | 0 |  | 247 | -20.06% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 13 | +116.67% |
| Render/plain_text | 0 |  | 1 |  | 0 |  | 5 | 0% |
| Load/strings | 154 | -8.33% | 96 | -9.43% | 0 |  | 2,732 | +2.44% |
| Render/strings | 494 | -1.59% | 609 | +0.66% | 0 |  | 1,430 | +1.49% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 159 | +19.55% |
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
