# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (100 so far, latest [65b7194](https://github.com/jinja2cpp/Jinja2Cpp/commit/65b7194c373607e507ab63cd7c65719507e79228) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 357,895 | -1.32% |  | 57 | -10.94% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 314,024 | -0.02% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 413,853 | -1.53% |  | 50 | -5.66% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 522,117 | -0.16% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 277,647 | -0.51% |  | 44 | -4.35% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 299,821 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,497 | -1.35% |  | 27 | -10.00% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,264,046 | 0% |  | 1,046 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 33,937 | -0.69% | -77.03% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 303,758 | 0% | -56.03% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,573 | -0.45% | -77.89% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 449,911 | 0% | -50.35% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,296 | -0.30% | -73.29% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,415 | 0% | -64.57% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,707 | -0.62% | -77.32% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 269,837 | 0% | -68.82% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,289 | -0.32% | -77.15% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 195,258 | 0% | -81.80% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,900 | -1.07% | -75.48% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,633 | 0% | -58.10% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,012 | -0.81% |  | 50 | -3.85% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,226,730 | 0% |  | 734 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 23,599 | -1.74% | -79.32% | 7 | -12.50% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,190 | -0.01% | -82.46% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 157,471 | -0.87% | -82.23% | 18 | -14.29% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,015 | 0% | -72.03% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,688 | -0.92% | -75.13% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 638,828 | 0% | -75.08% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,268,884 | -1.24% | -79.01% | 956 | -0.83% | -97.21% | 421,392 | -0.01% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 729,633 | +0.01% | -64.97% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,905 | -0.65% | -79.83% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,105,468 | 0% | -58.68% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,046 | -0.53% |  | 36 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,450,722 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,729 | -2.92% | -61.46% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | 0% | -85.29% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,701 | -1.13% | -74.75% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 608,317 | 0% | -71.00% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,868 | -1.63% | -73.65% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,219 | 0% | -74.83% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 730 | -5.19% | 727 | -3.84% | 0 |  | 6,793 | -5.68% |
| Render/chat_llama | 1,078 | -0.28% | 430 | +1.42% | 0 |  | 3,138 | -1.13% |
| Load/chat_mistral | 1,070 | -1.11% | 1,010 | -0.88% | 0 |  | 7,541 | -5.95% |
| Render/chat_mistral | 1,794 | -1.16% | 424 | -2.08% | 0 |  | 9,167 | +4.97% |
| Load/chat_qwen | 514 | -3.20% | 621 | -0.16% | 0 |  | 4,137 | -1.34% |
| Render/chat_qwen | 939 | 0% | 307 | -1.29% | 0 |  | 2,811 | -1.61% |
| Load/config_file | 180 | -2.17% | 187 | +3.31% | 0 |  | 2,846 | -4.43% |
| Render/config_file | 6,092 | -0.26% | 1,313 | -1.35% | 0 |  | 36,633 | -3.22% |
| Load/dict_ops | 31 | +6.90% | 16 | -15.79% | 0 |  | 1,248 | -2.27% |
| Render/dict_ops | 697 | -0.14% | 431 | -0.23% | 0 |  | 796 | -3.52% |
| Load/expressions | 30 | -18.92% | 25 | -3.85% | 0 |  | 1,199 | -1.96% |
| Render/expressions | 5 | 0% | 4 | +33.33% | 0 |  | 256 | -14.95% |
| Load/filters | 112 | -7.44% | 56 | -3.45% | 0 |  | 2,068 | -3.63% |
| Render/filters | 18 | +20.00% | 6 | -14.29% | 0 |  | 806 | -1.47% |
| Load/for_filter_if | 8 | 0% | 6 | 0% | 0 |  | 1,052 | +2.63% |
| Render/for_filter_if | 1,716 | +0.23% | 67 | 0% | 0 |  | 143 | -19.66% |
| Load/for_loop_vars | 32 | -3.03% | 21 | -4.55% | 0 |  | 1,038 | +1.96% |
| Render/for_loop_vars | 6 | 0% | 3 | 0% | 0 |  | 117 | +15.84% |
| Load/for_range | 3 | 0% | 5 | +25.00% | 0 |  | 734 | +0.14% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 78 | -16.13% |
| Load/html_autoescape | 483 | -3.59% | 502 | -1.95% | 0 |  | 3,993 | +0.63% |
| Render/html_autoescape | 2,847 | +2.08% | 1,106 | +0.45% | 0 |  | 30,916 | -2.18% |
| Load/inheritance | 2 | 0% | 3 | +50.00% | 0 |  | 626 | -4.86% |
| Render/inheritance | 569 | -0.18% | 134 | 0% | 0 |  | 754 | +1.07% |
| Load/large_static | 1,948 | -0.31% | 897 | -0.88% | 0 |  | 259 | -12.20% |
| Render/large_static | 700 | 0% | 576 | 0% | 0 |  | 7 | +40.00% |
| Load/macros | 69 | 0% | 61 | +7.02% | 0 |  | 1,627 | -2.11% |
| Render/macros | 715 | 0% | 241 | +0.42% | 0 |  | 1,047 | -53.07% |
| Load/many_tags | 27,607 | -3.68% | 21,464 | -3.44% | 0 |  | 239,891 | -6.59% |
| Render/many_tags | 6,032 | +0.25% | 881 | +1.38% | 0 |  | 96 | -21.95% |
| Load/mitsuhiko_table | 15 | -28.57% | 18 | -33.33% | 0 |  | 806 | -0.25% |
| Render/mitsuhiko_table | 9,040 | +0.01% | 6,607 | -0.03% | 0 |  | 291 | +1.39% |
| Load/mitsuhiko_table_wide | 69 | -12.66% | 74 | +2.78% | 0 |  | 962 | -10.76% |
| Render/mitsuhiko_table_wide | 9,188 | -0.07% | 22,920 | +0.01% | 0 |  | 298 | +5.30% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | -64.29% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 3 | -57.14% |
| Load/strings | 160 | +5.96% | 102 | +8.51% | 0 |  | 2,758 | -5.06% |
| Render/strings | 476 | -1.24% | 616 | 0% | 0 |  | 1,418 | -2.27% |
| Load/substitute | 2 | 0% | 5 | +66.67% | 0 |  | 146 | -9.32% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -66.67% |

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
