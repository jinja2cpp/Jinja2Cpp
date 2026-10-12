# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (84 so far, latest [dd9dd3d](https://github.com/jinja2cpp/Jinja2Cpp/commit/dd9dd3d81821d8a0722e729c556d455142930400) on 2026-10-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 353,386 | +0.86% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 310,222 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 421,643 | +0.47% |  | 56 | 0% |  | 21,464 | 0% | -51.93% |
| [Render/chat_mistral](#renderchat_mistral) | 496,848 | -0.01% |  | 360 | 0% |  | 8,536 | 0% | -35.37% |
| [Load/chat_qwen](#loadchat_qwen) | 274,357 | +1.29% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 297,344 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,668 | +0.31% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,335,584 | 0% |  | 1,071 | 0% |  | 23,384 | 0% | -1.78% |
| [Load/dict_ops](#loaddict_ops) | 33,365 | +0.12% | -77.42% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 301,094 | 0% | -56.42% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,363 | +0.20% | -78.48% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 445,924 | 0% | -50.79% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,756 | +0.19% | -73.85% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,503 | -8.19% | -64.47% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,193 | +0.17% | -77.70% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,616 | 0% | -68.15% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,682 | +0.27% | -77.48% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 193,597 | 0% | -81.95% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,752 | -0.03% | -75.74% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,030 | 0% | -58.76% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 197,584 | +0.32% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,254,091 | 0% |  | 748 | 0% |  | 42,984 | 0% | +6.44% |
| [Load/inheritance](#loadinheritance) | 23,285 | +0.33% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 136,194 | 0% | -81.79% | 32 | 0% | -94.86% | 6,440 | 0% | +7.05% |
| [Load/large_static](#loadlarge_static) | 155,313 | +0.03% | -82.47% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,095 | 0% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,562 | +0.38% | -75.63% | 28 | 0% | -84.95% | 4,280 | 0% | -31.15% |
| [Render/macros](#rendermacros) | 637,089 | 0% | -75.14% | 308 | 0% | -88.71% | 18,944 | 0% | +28.70% |
| [Load/many_tags](#loadmany_tags) | 9,106,623 | +0.32% | -79.38% | 945 | 0% | -97.24% | 421,280 | 0% | -60.08% |
| [Render/many_tags](#rendermany_tags) | 723,599 | 0% | -65.26% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,067 | +0.36% | -80.19% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,104,218 | 0% | -58.69% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,421 | +0.27% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,450,063 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,668 | -0.11% | -62.09% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 996 | 0% | -85.35% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,297 | +0.28% | -75.23% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 584,412 | 0% | -72.14% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,731 | -0.03% | -74.11% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,208 | 0% | -74.95% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 675 | -1.32% | 697 | -0.43% | 0 |  | 6,864 | +0.16% |
| Render/chat_llama | 1,063 | -0.65% | 422 | +0.96% | 0 |  | 3,365 | +5.32% |
| Load/chat_mistral | 1,075 | +1.03% | 1,019 | -0.39% | 0 |  | 7,771 | -2.87% |
| Render/chat_mistral | 1,752 | +1.15% | 453 | +1.80% | 0 |  | 9,178 | +10.95% |
| Load/chat_qwen | 549 | +3.98% | 592 | +1.54% | 0 |  | 4,300 | -2.23% |
| Render/chat_qwen | 927 | +0.76% | 297 | +0.34% | 0 |  | 2,834 | +2.38% |
| Load/config_file | 164 | -1.80% | 145 | +1.40% | 0 |  | 2,756 | -2.03% |
| Render/config_file | 5,489 | +1.74% | 1,434 | -3.24% | 0 |  | 39,048 | +5.10% |
| Load/dict_ops | 26 | +36.84% | 22 | +57.14% | 0 |  | 1,197 | -1.89% |
| Render/dict_ops | 608 | +2.18% | 410 | +0.49% | 0 |  | 811 | -1.34% |
| Load/expressions | 21 | -16.00% | 18 | -5.26% | 0 |  | 1,157 | +0.78% |
| Render/expressions | 4 | +100.00% | 2 | +100.00% | 0 |  | 293 | +18.15% |
| Load/filters | 108 | -1.82% | 83 | -1.19% | 0 |  | 1,911 | +12.61% |
| Render/filters | 34 | +88.89% | 7 | +75.00% | 0 |  | 831 | +1.71% |
| Load/for_filter_if | 10 | +42.86% | 5 | 0% | 0 |  | 1,044 | -3.24% |
| Render/for_filter_if | 1,579 | -0.38% | 61 | 0% | 0 |  | 163 | +13.99% |
| Load/for_loop_vars | 22 | -4.35% | 11 | 0% | 0 |  | 1,023 | -3.85% |
| Render/for_loop_vars | 12 | +71.43% | 6 | +200.00% | 0 |  | 108 | -8.47% |
| Load/for_range | 4 | -42.86% | 4 | -42.86% | 0 |  | 709 | +2.46% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 99 | +20.73% |
| Load/html_autoescape | 437 | +1.16% | 495 | 0% | 0 |  | 3,842 | -2.36% |
| Render/html_autoescape | 4,216 | +28.03% | 1,223 | +1.49% | 0 |  | 36,958 | +4.02% |
| Load/inheritance | 1 | -83.33% | 3 | -25.00% | 0 |  | 620 | +8.01% |
| Render/inheritance | 564 | 0% | 117 | -5.65% | 0 |  | 703 | +1.15% |
| Load/large_static | 1,963 | +0.46% | 887 | +0.68% | 0 |  | 244 | +15.64% |
| Render/large_static | 705 | 0% | 573 | 0% | 0 |  | 3 | -84.21% |
| Load/macros | 76 | -7.32% | 63 | -12.50% | 0 |  | 1,544 | +0.59% |
| Render/macros | 856 | +12.48% | 241 | +1.26% | 0 |  | 1,312 | +166.67% |
| Load/many_tags | 26,913 | +1.07% | 20,593 | +0.09% | 0 |  | 233,478 | -6.10% |
| Render/many_tags | 6,039 | -0.40% | 856 | -0.58% | 0 |  | 111 | -79.21% |
| Load/mitsuhiko_table | 28 | 0% | 29 | -6.45% | 0 |  | 767 | +3.23% |
| Render/mitsuhiko_table | 9,302 | +0.36% | 6,503 | 0% | 0 |  | 293 | 0% |
| Load/mitsuhiko_table_wide | 94 | -3.09% | 110 | +1.85% | 0 |  | 1,236 | -5.50% |
| Render/mitsuhiko_table_wide | 9,396 | +0.22% | 22,970 | +0.01% | 0 |  | 273 | -10.78% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 13 | +116.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 143 | -2.72% | 117 | +1.74% | 0 |  | 2,601 | -1.51% |
| Render/strings | 459 | +0.44% | 601 | +0.17% | 0 |  | 1,448 | +2.33% |
| Load/substitute | 1 | -50.00% | 3 | -25.00% | 0 |  | 113 | +15.31% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -25.00% |

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
