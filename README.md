# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (76 so far, latest [8f13350](https://github.com/jinja2cpp/Jinja2Cpp/commit/8f13350dd8b53325a1cd129c0ada805e8b73e718) on 2026-10-12). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 350,379 | +0.15% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 310,222 | -0.13% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 419,650 | +0.14% |  | 56 | 0% |  | 21,464 | 0% | -51.93% |
| [Render/chat_mistral](#renderchat_mistral) | 496,908 | -0.71% |  | 360 | 0% |  | 8,536 | 0% | -35.37% |
| [Load/chat_qwen](#loadchat_qwen) | 270,856 | -0.01% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 297,344 | -0.24% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,357 | -0.18% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,335,584 | -0.01% |  | 1,071 | 0% |  | 23,384 | 0% | -1.78% |
| [Load/dict_ops](#loaddict_ops) | 33,325 | +0.26% | -77.45% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 301,094 | +0.09% | -56.42% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,275 | +0.24% | -78.52% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 445,924 | -0.08% | -50.79% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,617 | +0.13% | -73.90% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 34,315 | -0.07% | -61.30% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,141 | +0.04% | -77.74% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 275,616 | 0% | -68.15% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,573 | -0.25% | -77.54% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 193,597 | -0.08% | -81.95% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,756 | +0.32% | -75.73% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,030 | -0.43% | -58.76% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 196,952 | -0.13% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,254,091 | +0.40% |  | 748 | 0% |  | 42,984 | 0% | +6.44% |
| [Load/inheritance](#loadinheritance) | 23,209 | +0.13% | -79.66% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 136,194 | +0.48% | -81.79% | 32 | 0% | -94.86% | 6,440 | 0% | +7.05% |
| [Load/large_static](#loadlarge_static) | 155,267 | +0.68% | -82.48% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,095 | +0.02% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,353 | +0.07% | -75.72% | 28 | 0% | -84.95% | 4,280 | 0% | -31.15% |
| [Render/macros](#rendermacros) | 637,089 | -2.50% | -75.14% | 308 | 0% | -88.71% | 18,944 | 0% | +28.70% |
| [Load/many_tags](#loadmany_tags) | 9,077,530 | +0.03% | -79.45% | 945 | 0% | -97.24% | 421,280 | 0% | -60.08% |
| [Render/many_tags](#rendermany_tags) | 723,599 | -10.76% | -65.26% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,899 | +0.30% | -80.26% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,104,218 | +0.28% | -58.69% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,239 | +0.20% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,450,063 | +0.26% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,672 | +0.14% | -62.05% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 996 | +0.10% | -85.35% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,095 | +0.08% | -75.30% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 584,433 | +0.19% | -72.14% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,733 | +0.64% | -74.10% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,208 | +0.41% | -74.95% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 679 | +1.49% | 699 | +0.72% | 0 |  | 6,887 | -3.49% |
| Render/chat_llama | 1,086 | +0.18% | 421 | +3.44% | 0 |  | 3,209 | -2.99% |
| Load/chat_mistral | 1,072 | -0.83% | 1,022 | -0.20% | 0 |  | 8,184 | -2.53% |
| Render/chat_mistral | 1,735 | -1.25% | 444 | +5.97% | 0 |  | 8,313 | -5.87% |
| Load/chat_qwen | 523 | -1.69% | 586 | -0.17% | 0 |  | 4,515 | +3.79% |
| Render/chat_qwen | 956 | -5.35% | 304 | +2.01% | 0 |  | 2,753 | -9.47% |
| Load/config_file | 170 | +6.25% | 140 | +3.70% | 0 |  | 2,821 | +3.14% |
| Render/config_file | 5,481 | +2.12% | 1,509 | +13.20% | 0 |  | 36,324 | -7.23% |
| Load/dict_ops | 22 | 0% | 16 | +14.29% | 0 |  | 1,227 | -1.13% |
| Render/dict_ops | 605 | +0.83% | 406 | +1.75% | 0 |  | 808 | -1.94% |
| Load/expressions | 30 | +87.50% | 22 | +57.14% | 0 |  | 1,164 | -1.19% |
| Render/expressions | 5 | -16.67% | 2 | 0% | 0 |  | 291 | -3.64% |
| Load/filters | 115 | +2.68% | 85 | +3.66% | 0 |  | 1,876 | +4.22% |
| Render/filters | 14 | -22.22% | 2 | -33.33% | 0 |  | 829 | -2.81% |
| Load/for_filter_if | 11 | 0% | 5 | -16.67% | 0 |  | 1,084 | +2.94% |
| Render/for_filter_if | 1,587 | +0.19% | 61 | 0% | 0 |  | 156 | +20.00% |
| Load/for_loop_vars | 21 | +5.00% | 12 | +33.33% | 0 |  | 1,079 | +3.15% |
| Render/for_loop_vars | 8 | 0% | 2 | -33.33% | 0 |  | 174 | +37.01% |
| Load/for_range | 8 | +60.00% | 7 | +40.00% | 0 |  | 696 | -2.93% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 93 | +3.33% |
| Load/html_autoescape | 439 | +1.39% | 494 | 0% | 0 |  | 4,050 | +3.21% |
| Render/html_autoescape | 3,403 | +23.07% | 1,294 | +21.50% | 0 |  | 34,993 | -3.71% |
| Load/inheritance | 5 | 0% | 4 | 0% | 0 |  | 586 | +1.74% |
| Render/inheritance | 565 | +3.29% | 126 | +7.69% | 0 |  | 707 | -6.97% |
| Load/large_static | 1,953 | +0.05% | 881 | 0% | 0 |  | 266 | +8.13% |
| Render/large_static | 705 | -0.14% | 573 | 0% | 0 |  | 3 | 0% |
| Load/macros | 83 | +12.16% | 73 | +5.80% | 0 |  | 1,593 | +1.79% |
| Render/macros | 760 | +0.80% | 238 | +2.15% | 0 |  | 495 | -57.11% |
| Load/many_tags | 26,632 | +0.48% | 20,571 | +0.02% | 0 |  | 239,223 | +1.45% |
| Render/many_tags | 6,066 | +0.48% | 860 | +24.82% | 0 |  | 74 | -40.32% |
| Load/mitsuhiko_table | 26 | +13.04% | 24 | +26.32% | 0 |  | 781 | +4.27% |
| Render/mitsuhiko_table | 9,286 | +0.13% | 6,505 | +0.12% | 0 |  | 296 | +8.42% |
| Load/mitsuhiko_table_wide | 95 | +3.26% | 109 | -1.80% | 0 |  | 1,198 | +4.54% |
| Render/mitsuhiko_table_wide | 9,370 | +0.14% | 22,968 | +0.02% | 0 |  | 290 | +1.05% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 15 | +66.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/strings | 141 | -2.08% | 111 | +2.78% | 0 |  | 2,710 | +0.52% |
| Render/strings | 465 | -1.69% | 600 | -0.17% | 0 |  | 1,401 | -1.82% |
| Load/substitute | 1 | 0% | 4 | +33.33% | 0 |  | 107 | -14.40% |
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
