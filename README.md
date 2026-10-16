# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (95 so far, latest [e9c839b](https://github.com/jinja2cpp/Jinja2Cpp/commit/e9c839ba30714a79fb3bd3b56717cbfe4d7447d8) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,418 | -0.02% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 309,805 | +0.50% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,323 | +0.01% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 518,617 | +0.37% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 275,872 | +0.02% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 295,806 | +0.41% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,498 | -0.02% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,260,520 | +0.44% |  | 1,046 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 33,658 | +0.04% | -77.22% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 304,656 | +0.06% | -55.90% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,277 | 0% | -78.04% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 460,560 | 0% | -49.17% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,985 | +0.04% | -73.41% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,184 | +0.10% | -64.83% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,375 | -0.02% | -77.56% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,489 | 0% | -68.63% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,850 | -0.02% | -77.39% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 195,231 | +0.51% | -81.80% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,760 | 0% | -75.72% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,627 | 0% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 199,324 | -0.02% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,225,795 | +0.03% |  | 734 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 23,289 | 0% | -79.59% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,051 | -0.01% | -82.48% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 156,406 | 0% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,015 | 0% | -72.03% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,100 | +0.02% | -75.39% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 643,641 | 0% | -74.89% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,196,437 | 0% | -79.18% | 945 | 0% | -97.24% | 421,440 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 728,459 | 0% | -65.03% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,470 | 0% | -80.02% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,106,308 | 0% | -58.67% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,425 | 0% |  | 36 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,451,562 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,702 | 0% | -61.74% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | 0% | -85.29% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,191 | +0.05% | -74.92% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 602,024 | +0.24% | -71.30% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,797 | 0% | -73.89% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,219 | 0% | -74.83% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 700 | +0.57% | 723 | +0.42% | 0 |  | 7,608 | +5.92% |
| Render/chat_llama | 1,049 | -1.04% | 423 | -4.94% | 0 |  | 3,324 | -7.59% |
| Load/chat_mistral | 1,021 | +0.10% | 986 | -0.20% | 0 |  | 8,653 | +9.20% |
| Render/chat_mistral | 1,742 | -4.02% | 415 | -10.75% | 0 |  | 9,288 | -2.47% |
| Load/chat_qwen | 489 | -0.81% | 606 | -0.33% | 0 |  | 4,789 | +6.78% |
| Render/chat_qwen | 927 | -1.90% | 297 | -9.73% | 0 |  | 2,807 | -2.43% |
| Load/config_file | 180 | +5.26% | 174 | +2.35% | 0 |  | 2,898 | +3.21% |
| Render/config_file | 5,640 | -0.35% | 1,413 | +0.28% | 0 |  | 36,403 | +0.83% |
| Load/dict_ops | 26 | -13.33% | 15 | 0% | 0 |  | 1,281 | +3.06% |
| Render/dict_ops | 703 | +0.43% | 425 | 0% | 0 |  | 830 | +0.73% |
| Load/expressions | 25 | +13.64% | 15 | +15.38% | 0 |  | 1,226 | +0.91% |
| Render/expressions | 5 | 0% | 4 | 0% | 0 |  | 272 | +7.51% |
| Load/filters | 114 | 0% | 53 | -3.64% | 0 |  | 2,131 | +18.19% |
| Render/filters | 19 | -20.83% | 6 | -25.00% | 0 |  | 769 | -2.78% |
| Load/for_filter_if | 4 | 0% | 3 | 0% | 0 |  | 1,077 | +1.51% |
| Render/for_filter_if | 1,714 | 0% | 63 | -1.56% | 0 |  | 156 | +10.64% |
| Load/for_loop_vars | 26 | +36.84% | 14 | 0% | 0 |  | 1,073 | +2.09% |
| Render/for_loop_vars | 4 | -20.00% | 2 | 0% | 0 |  | 154 | +16.67% |
| Load/for_range | 3 | +50.00% | 4 | 0% | 0 |  | 705 | +1.59% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 58 | -32.56% |
| Load/html_autoescape | 473 | +0.21% | 490 | -1.21% | 0 |  | 4,092 | +8.08% |
| Render/html_autoescape | 3,030 | -15.43% | 1,013 | -12.37% | 0 |  | 33,388 | +5.56% |
| Load/inheritance | 1 | -50.00% | 2 | 0% | 0 |  | 613 | -3.31% |
| Render/inheritance | 561 | -0.36% | 130 | +0.78% | 0 |  | 760 | -4.16% |
| Load/large_static | 1,943 | -0.10% | 887 | 0% | 0 |  | 252 | +18.87% |
| Render/large_static | 700 | 0% | 576 | 0% | 0 |  | 4 | -86.21% |
| Load/macros | 70 | +4.48% | 60 | 0% | 0 |  | 1,599 | +3.03% |
| Render/macros | 712 | -0.28% | 241 | +0.84% | 0 |  | 2,169 | +298.71% |
| Load/many_tags | 26,665 | -0.84% | 20,463 | -0.03% | 0 |  | 254,296 | +13.70% |
| Render/many_tags | 6,025 | -0.08% | 851 | +0.12% | 0 |  | 145 | +22.88% |
| Load/mitsuhiko_table | 18 | -10.00% | 23 | 0% | 0 |  | 844 | +10.04% |
| Render/mitsuhiko_table | 9,043 | -0.02% | 6,607 | +0.03% | 0 |  | 292 | +2.82% |
| Load/mitsuhiko_table_wide | 65 | -4.41% | 68 | -6.85% | 0 |  | 1,069 | +16.83% |
| Render/mitsuhiko_table_wide | 9,191 | -0.05% | 22,916 | -0.01% | 0 |  | 278 | -3.81% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | -16.67% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 4 | -33.33% |
| Load/strings | 146 | -3.31% | 95 | +3.26% | 0 |  | 2,790 | +4.10% |
| Render/strings | 478 | -0.62% | 614 | -0.65% | 0 |  | 1,425 | -31.06% |
| Load/substitute | 2 | 0% | 3 | -25.00% | 0 |  | 131 | +7.38% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -70.00% |

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
