# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (86 so far, latest [f4e312c](https://github.com/jinja2cpp/Jinja2Cpp/commit/f4e312c7e7e1442f1f508f9e678b978c818e0410) on 2026-10-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 356,772 | +0.89% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 308,324 | -0.62% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 412,743 | -2.26% |  | 52 | -7.14% |  | 20,664 | -3.73% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 514,959 | +3.63% |  | 450 | +25.00% |  | 9,040 | +5.90% | -31.56% |
| [Load/chat_qwen](#loadchat_qwen) | 276,763 | +0.83% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 293,570 | -1.28% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,743 | +1.03% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,332,151 | -0.24% |  | 1,071 | 0% |  | 23,400 | +0.07% | -1.71% |
| [Load/dict_ops](#loaddict_ops) | 33,695 | +0.83% | -77.20% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 302,171 | +0.38% | -56.26% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,306 | +2.08% | -78.02% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,706 | +0.36% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,683 | +1.28% | -73.52% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,587 | +0.19% | -64.38% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,450 | +0.38% | -77.51% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 270,662 | -1.99% | -68.73% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,949 | +0.59% | -77.33% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,237 | +0.28% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,783 | +0.04% | -75.68% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,625 | +1.31% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 199,047 | +0.70% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,254,990 | +0.07% |  | 748 | 0% |  | 42,984 | 0% | +6.44% |
| [Load/inheritance](#loadinheritance) | 23,403 | +0.50% | -79.49% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 134,641 | -1.17% | -82.00% | 32 | 0% | -94.86% | 6,440 | 0% | +7.05% |
| [Load/large_static](#loadlarge_static) | 156,578 | +0.81% | -82.33% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,096 | +0.01% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,967 | +0.68% | -75.45% | 28 | 0% | -84.95% | 4,280 | 0% | -31.15% |
| [Render/macros](#rendermacros) | 637,032 | -0.02% | -75.15% | 308 | 0% | -88.71% | 18,944 | 0% | +28.70% |
| [Load/many_tags](#loadmany_tags) | 9,212,863 | +1.12% | -79.14% | 945 | 0% | -97.24% | 421,312 | +0.01% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 717,726 | -0.81% | -65.54% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,548 | +0.85% | -79.99% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,112,030 | +0.02% | -58.63% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,627 | +0.19% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,457,371 | +0.01% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,689 | +0.57% | -61.88% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 997 | +0.12% | -85.33% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,217 | +1.14% | -74.91% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 596,454 | +2.02% | -71.56% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,800 | +0.89% | -73.88% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,224 | +0.73% | -74.77% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 668 | -1.47% | 682 | -2.43% | 0 |  | 7,444 | +14.17% |
| Render/chat_llama | 1,109 | +4.23% | 428 | +0.23% | 0 |  | 3,369 | +1.35% |
| Load/chat_mistral | 993 | -7.80% | 965 | -5.30% | 0 |  | 8,468 | +15.54% |
| Render/chat_mistral | 1,682 | -4.76% | 442 | -5.96% | 0 |  | 9,220 | +5.32% |
| Load/chat_qwen | 512 | -6.91% | 601 | +1.35% | 0 |  | 4,688 | +18.74% |
| Render/chat_qwen | 1,009 | +8.96% | 343 | +12.46% | 0 |  | 2,736 | -0.33% |
| Load/config_file | 180 | +7.78% | 168 | +14.29% | 0 |  | 2,979 | +14.31% |
| Render/config_file | 5,713 | +4.16% | 1,452 | -0.55% | 0 |  | 39,456 | -1.32% |
| Load/dict_ops | 37 | +54.17% | 27 | +28.57% | 0 |  | 1,258 | +4.05% |
| Render/dict_ops | 655 | +7.73% | 416 | +1.71% | 0 |  | 833 | +0.48% |
| Load/expressions | 37 | +37.04% | 37 | +68.18% | 0 |  | 1,196 | +1.36% |
| Render/expressions | 14 | +250.00% | 7 | +250.00% | 0 |  | 266 | +1.92% |
| Load/filters | 131 | +15.93% | 73 | -13.10% | 0 |  | 1,930 | +9.35% |
| Render/filters | 28 | -17.65% | 4 | -42.86% | 0 |  | 811 | -3.80% |
| Load/for_filter_if | 14 | +40.00% | 11 | +120.00% | 0 |  | 1,103 | +8.46% |
| Render/for_filter_if | 1,721 | +8.92% | 60 | 0% | 0 |  | 155 | +9.93% |
| Load/for_loop_vars | 28 | +33.33% | 18 | +63.64% | 0 |  | 1,099 | +11.46% |
| Render/for_loop_vars | 5 | -58.33% | 1 | -80.00% | 0 |  | 136 | +29.52% |
| Load/for_range | 11 | +175.00% | 10 | +100.00% | 0 |  | 728 | +2.25% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 92 | -8.91% |
| Load/html_autoescape | 437 | 0% | 493 | -0.60% | 0 |  | 4,368 | +25.30% |
| Render/html_autoescape | 4,152 | -0.02% | 1,344 | +5.08% | 0 |  | 35,502 | -7.48% |
| Load/inheritance | 6 | +500.00% | 5 | +66.67% | 0 |  | 634 | +9.12% |
| Render/inheritance | 561 | -1.23% | 129 | +9.32% | 0 |  | 729 | +2.68% |
| Load/large_static | 1,953 | -0.51% | 891 | +0.45% | 0 |  | 268 | +3.47% |
| Render/large_static | 704 | -0.14% | 572 | -0.17% | 0 |  | 15 | +400.00% |
| Load/macros | 81 | +8.00% | 52 | -16.13% | 0 |  | 1,630 | +8.38% |
| Render/macros | 760 | -11.11% | 239 | -1.24% | 0 |  | 1,969 | +289.90% |
| Load/many_tags | 27,048 | -0.07% | 20,588 | -0.08% | 0 |  | 254,574 | +15.46% |
| Render/many_tags | 6,057 | +0.31% | 855 | -0.23% | 0 |  | 128 | +23.08% |
| Load/mitsuhiko_table | 24 | -4.00% | 23 | -20.69% | 0 |  | 834 | +12.40% |
| Render/mitsuhiko_table | 9,271 | -0.32% | 6,608 | +1.52% | 0 |  | 313 | +2.62% |
| Load/mitsuhiko_table_wide | 88 | -8.33% | 113 | 0% | 0 |  | 1,194 | +2.58% |
| Render/mitsuhiko_table_wide | 9,362 | -0.36% | 22,905 | -0.30% | 0 |  | 295 | -1.34% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -71.43% |
| Render/plain_text | 0 |  | 1 |  | 0 |  | 5 | 0% |
| Load/strings | 154 | +6.94% | 92 | -21.37% | 0 |  | 2,768 | +7.33% |
| Render/strings | 489 | +6.30% | 610 | +1.33% | 0 |  | 2,254 | +62.16% |
| Load/substitute | 2 | +100.00% | 3 | 0% | 0 |  | 130 | +7.44% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 11 | +266.67% |

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
