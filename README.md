# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (97 so far, latest [bde921f](https://github.com/jinja2cpp/Jinja2Cpp/commit/bde921f4ea9071f68121bf40ab7c3cfff236842f) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 360,479 | +1.43% |  | 64 | +8.47% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 310,042 | +0.01% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 418,173 | +1.67% |  | 53 | +1.92% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 520,112 | +0.16% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 277,439 | +0.56% |  | 46 | +2.22% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 295,861 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 102,302 | +1.78% |  | 30 | +11.11% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,261,792 | +0.02% |  | 1,046 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 33,949 | +0.86% | -77.03% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 304,656 | 0% | -55.90% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,551 | +0.61% | -77.90% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 462,760 | 0% | -48.93% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,153 | +0.27% | -73.34% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,141 | 0% | -64.88% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,696 | +1.01% | -77.33% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,489 | 0% | -68.63% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,152 | +0.74% | -77.22% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 195,231 | 0% | -81.80% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,948 | +1.37% | -75.39% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,627 | 0% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,561 | +1.10% |  | 52 | +4.00% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,225,784 | 0% |  | 734 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 23,853 | +2.42% | -79.10% | 8 | +14.29% | -92.52% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,084 | +0.01% | -82.47% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 158,003 | +1.02% | -82.17% | 21 | +16.67% | -95.23% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,015 | 0% | -72.03% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,942 | +1.50% | -75.02% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 643,650 | 0% | -74.89% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,331,858 | +1.49% | -78.87% | 964 | +2.01% | -97.19% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 728,339 | -0.02% | -65.03% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,865 | +0.83% | -79.85% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,106,297 | 0% | -58.67% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,820 | +0.70% |  | 36 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,451,551 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,800 | +2.65% | -60.73% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | 0% | -85.29% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,130 | +1.30% | -74.60% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 602,013 | 0% | -71.30% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,921 | +1.59% | -73.47% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,219 | 0% | -74.83% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 775 | +13.97% | 754 | +5.16% | 0 |  | 7,474 | +2.05% |
| Render/chat_llama | 1,069 | +2.99% | 409 | -1.92% | 0 |  | 3,306 | -5.54% |
| Load/chat_mistral | 1,118 | +12.02% | 1,025 | +3.74% | 0 |  | 8,350 | +2.14% |
| Render/chat_mistral | 1,788 | +3.53% | 414 | +0.73% | 0 |  | 9,476 | -1.50% |
| Load/chat_qwen | 528 | +7.76% | 626 | +3.30% | 0 |  | 4,681 | +0.47% |
| Render/chat_qwen | 918 | +1.21% | 296 | -1.00% | 0 |  | 2,794 | -4.77% |
| Load/config_file | 194 | +12.79% | 184 | +3.95% | 0 |  | 3,049 | +6.39% |
| Render/config_file | 6,030 | +3.86% | 1,338 | -4.36% | 0 |  | 37,406 | +1.08% |
| Load/dict_ops | 20 | -16.67% | 14 | +7.69% | 0 |  | 1,242 | +0.16% |
| Render/dict_ops | 698 | +1.01% | 433 | +0.70% | 0 |  | 845 | +1.68% |
| Load/expressions | 24 | +9.09% | 17 | +30.77% | 0 |  | 1,257 | +0.48% |
| Render/expressions | 4 | +100.00% | 3 | +200.00% | 0 |  | 299 | +8.73% |
| Load/filters | 118 | +7.27% | 57 | +9.62% | 0 |  | 1,957 | +8.84% |
| Render/filters | 18 | +125.00% | 5 | +66.67% | 0 |  | 786 | -1.13% |
| Load/for_filter_if | 13 | +225.00% | 8 | +166.67% | 0 |  | 1,076 | -1.10% |
| Render/for_filter_if | 1,712 | +0.06% | 63 | -7.35% | 0 |  | 136 | -19.53% |
| Load/for_loop_vars | 25 | +8.70% | 17 | +6.25% | 0 |  | 1,045 | -4.13% |
| Render/for_loop_vars | 6 | +50.00% | 3 | +50.00% | 0 |  | 118 | -28.05% |
| Load/for_range | 3 | 0% | 3 | -25.00% | 0 |  | 732 | +5.02% |
| Render/for_range | 4 | +300.00% | 3 | +200.00% | 0 |  | 95 | -27.48% |
| Load/html_autoescape | 501 | +4.81% | 510 | +4.29% | 0 |  | 4,232 | +6.49% |
| Render/html_autoescape | 3,066 | +11.09% | 1,195 | +10.65% | 0 |  | 31,969 | -3.80% |
| Load/inheritance | 2 | +100.00% | 2 | -33.33% | 0 |  | 668 | +5.36% |
| Render/inheritance | 568 | +0.71% | 132 | +0.76% | 0 |  | 764 | -1.55% |
| Load/large_static | 1,961 | +0.93% | 904 | +1.92% | 0 |  | 310 | +18.32% |
| Render/large_static | 700 | 0% | 576 | 0% | 0 |  | 3 | -81.25% |
| Load/macros | 68 | +3.03% | 57 | -6.56% | 0 |  | 1,662 | +5.79% |
| Render/macros | 713 | +0.14% | 239 | 0% | 0 |  | 507 | -5.76% |
| Load/many_tags | 28,367 | +5.90% | 22,115 | +7.88% | 0 |  | 244,256 | +0.51% |
| Render/many_tags | 6,012 | -0.27% | 860 | 0% | 0 |  | 104 | -32.47% |
| Load/mitsuhiko_table | 18 | 0% | 25 | -10.71% | 0 |  | 778 | -2.87% |
| Render/mitsuhiko_table | 9,031 | -0.27% | 6,608 | +0.03% | 0 |  | 261 | -6.79% |
| Load/mitsuhiko_table_wide | 70 | +16.67% | 70 | +14.75% | 0 |  | 886 | -4.11% |
| Render/mitsuhiko_table_wide | 9,181 | 0% | 22,918 | -0.01% | 0 |  | 257 | -10.76% |
| Load/plain_text | 0 |  | 3 | +50.00% | 0 |  | 14 | -30.00% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 6 | 0% |
| Load/strings | 149 | +5.67% | 97 | +3.19% | 0 |  | 2,816 | +3.45% |
| Render/strings | 473 | -2.87% | 615 | +0.16% | 0 |  | 1,433 | -0.35% |
| Load/substitute | 2 | +100.00% | 4 | +33.33% | 0 |  | 143 | +8.33% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |

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
