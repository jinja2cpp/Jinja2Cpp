# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (98 so far, latest [f11b3d2](https://github.com/jinja2cpp/Jinja2Cpp/commit/f11b3d29d7d7392f5fe48d8b4bb6285455bedecb) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 362,194 | +0.48% |  | 64 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 310,053 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 420,065 | +0.45% |  | 53 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 520,112 | 0% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 278,726 | +0.46% |  | 46 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 295,861 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 102,868 | +0.55% |  | 30 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,261,983 | +0.02% |  | 1,046 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 34,168 | +0.65% | -76.88% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 304,664 | 0% | -55.90% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,804 | +0.56% | -77.78% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 462,760 | 0% | -48.93% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,494 | +0.47% | -73.22% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,158 | +0.05% | -64.86% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,896 | +0.65% | -77.18% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,489 | 0% | -68.63% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,419 | +0.65% | -77.07% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 195,231 | 0% | -81.80% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 14,049 | +0.72% | -75.22% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,627 | 0% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 202,557 | +0.49% |  | 52 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,224,644 | -0.09% |  | 734 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 24,017 | +0.69% | -78.95% | 8 | 0% | -92.52% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,084 | 0% | -82.47% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 158,851 | +0.54% | -82.07% | 21 | 0% | -95.23% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,015 | 0% | -72.03% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 57,207 | +0.47% | -74.90% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 643,650 | 0% | -74.89% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,383,148 | +0.55% | -78.75% | 964 | 0% | -97.19% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 728,339 | 0% | -65.03% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 48,221 | +0.74% | -79.70% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,106,308 | 0% | -58.67% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,236 | +0.73% |  | 36 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,451,562 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,842 | +1.11% | -60.29% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | 0% | -85.29% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,519 | +0.52% | -74.47% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 602,020 | 0% | -71.30% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,999 | +0.98% | -73.21% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,219 | 0% | -74.83% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 772 | -0.39% | 752 | -0.27% | 0 |  | 7,372 | -1.36% |
| Render/chat_llama | 1,066 | -0.28% | 412 | +0.73% | 0 |  | 3,447 | +4.26% |
| Load/chat_mistral | 1,091 | -2.42% | 1,022 | -0.29% | 0 |  | 8,371 | +0.25% |
| Render/chat_mistral | 1,784 | -0.22% | 407 | -1.69% | 0 |  | 9,837 | +3.81% |
| Load/chat_qwen | 525 | -0.57% | 620 | -0.96% | 0 |  | 4,397 | -6.07% |
| Render/chat_qwen | 934 | +1.74% | 299 | +1.01% | 0 |  | 2,821 | +0.97% |
| Load/config_file | 183 | -5.67% | 185 | +0.54% | 0 |  | 3,088 | +1.28% |
| Render/config_file | 5,993 | -0.61% | 1,310 | -2.09% | 0 |  | 38,718 | +3.51% |
| Load/dict_ops | 28 | +40.00% | 17 | +21.43% | 0 |  | 1,312 | +5.64% |
| Render/dict_ops | 700 | +0.29% | 434 | +0.23% | 0 |  | 863 | +2.13% |
| Load/expressions | 31 | +29.17% | 20 | +17.65% | 0 |  | 1,240 | -1.35% |
| Render/expressions | 6 | +50.00% | 3 | 0% | 0 |  | 299 | 0% |
| Load/filters | 122 | +3.39% | 61 | +7.02% | 0 |  | 2,072 | +5.88% |
| Render/filters | 12 | -33.33% | 5 | 0% | 0 |  | 786 | 0% |
| Load/for_filter_if | 15 | +15.38% | 8 | 0% | 0 |  | 1,100 | +2.23% |
| Render/for_filter_if | 1,712 | 0% | 63 | 0% | 0 |  | 119 | -12.50% |
| Load/for_loop_vars | 31 | +24.00% | 19 | +11.76% | 0 |  | 1,102 | +5.45% |
| Render/for_loop_vars | 6 | 0% | 3 | 0% | 0 |  | 115 | -2.54% |
| Load/for_range | 3 | 0% | 4 | +33.33% | 0 |  | 743 | +1.50% |
| Render/for_range | 3 | -25.00% | 3 | 0% | 0 |  | 90 | -5.26% |
| Load/html_autoescape | 503 | +0.40% | 511 | +0.20% | 0 |  | 4,248 | +0.38% |
| Render/html_autoescape | 2,877 | -6.16% | 1,144 | -4.27% | 0 |  | 33,703 | +5.42% |
| Load/inheritance | 2 | 0% | 2 | 0% | 0 |  | 645 | -3.44% |
| Render/inheritance | 562 | -1.06% | 131 | -0.76% | 0 |  | 757 | -0.92% |
| Load/large_static | 1,955 | -0.31% | 905 | +0.11% | 0 |  | 292 | -5.81% |
| Render/large_static | 700 | 0% | 576 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 69 | +1.47% | 58 | +1.75% | 0 |  | 1,717 | +3.31% |
| Render/macros | 714 | +0.14% | 237 | -0.84% | 0 |  | 1,524 | +200.59% |
| Load/many_tags | 28,487 | +0.42% | 22,166 | +0.23% | 0 |  | 261,866 | +7.21% |
| Render/many_tags | 6,015 | +0.05% | 863 | +0.35% | 0 |  | 127 | +22.12% |
| Load/mitsuhiko_table | 19 | +5.56% | 24 | -4.00% | 0 |  | 842 | +8.23% |
| Render/mitsuhiko_table | 9,036 | +0.06% | 6,607 | -0.02% | 0 |  | 291 | +11.49% |
| Load/mitsuhiko_table_wide | 80 | +14.29% | 74 | +5.71% | 0 |  | 973 | +9.82% |
| Render/mitsuhiko_table_wide | 9,192 | +0.12% | 22,919 | 0% | 0 |  | 289 | +12.45% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | -64.29% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| Load/strings | 150 | +0.67% | 97 | 0% | 0 |  | 2,847 | +1.10% |
| Render/strings | 474 | +0.21% | 614 | -0.16% | 0 |  | 1,444 | +0.77% |
| Load/substitute | 2 | 0% | 3 | -25.00% | 0 |  | 154 | +7.69% |
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
