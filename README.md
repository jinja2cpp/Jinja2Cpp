# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (96 so far, latest [84a70a4](https://github.com/jinja2cpp/Jinja2Cpp/commit/84a70a4324c5fcb2a2830708054b5d9025c42a0d) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,394 | -0.01% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 310,014 | +0.07% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,300 | -0.01% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 519,277 | +0.13% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 275,881 | 0% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 295,861 | +0.02% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,512 | +0.01% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,261,504 | +0.08% |  | 1,046 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 33,658 | 0% | -77.22% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 304,656 | 0% | -55.90% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,277 | 0% | -78.04% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 462,760 | +0.48% | -48.93% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,955 | -0.04% | -73.42% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,141 | -0.14% | -64.88% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,389 | +0.05% | -77.55% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,489 | 0% | -68.63% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,850 | 0% | -77.39% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 195,231 | 0% | -81.80% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,760 | 0% | -75.72% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,627 | 0% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 199,367 | +0.02% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,225,784 | 0% |  | 734 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 23,289 | 0% | -79.59% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 131,069 | +0.01% | -82.47% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 156,406 | 0% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,015 | 0% | -72.03% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,100 | 0% | -75.39% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 643,650 | 0% | -74.89% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,194,637 | -0.02% | -79.18% | 945 | 0% | -97.24% | 421,440 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 728,459 | 0% | -65.03% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,470 | 0% | -80.02% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,106,297 | 0% | -58.67% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,425 | 0% |  | 36 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,451,551 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,702 | 0% | -61.74% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | 0% | -85.29% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,179 | -0.02% | -74.93% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 602,013 | 0% | -71.30% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,797 | 0% | -73.89% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,219 | 0% | -74.83% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 680 | -2.86% | 717 | -0.83% | 0 |  | 7,324 | -3.73% |
| Render/chat_llama | 1,038 | -1.05% | 417 | -1.42% | 0 |  | 3,500 | +5.29% |
| Load/chat_mistral | 998 | -2.25% | 988 | +0.20% | 0 |  | 8,175 | -5.52% |
| Render/chat_mistral | 1,727 | -0.86% | 411 | -0.96% | 0 |  | 9,620 | +3.57% |
| Load/chat_qwen | 490 | +0.20% | 606 | 0% | 0 |  | 4,659 | -2.71% |
| Render/chat_qwen | 907 | -2.16% | 299 | +0.67% | 0 |  | 2,934 | +4.52% |
| Load/config_file | 172 | -4.44% | 177 | +1.72% | 0 |  | 2,866 | -1.10% |
| Render/config_file | 5,806 | +2.94% | 1,399 | -0.99% | 0 |  | 37,008 | +1.66% |
| Load/dict_ops | 24 | -7.69% | 13 | -13.33% | 0 |  | 1,240 | -3.20% |
| Render/dict_ops | 691 | -1.71% | 430 | +1.18% | 0 |  | 831 | +0.12% |
| Load/expressions | 22 | -12.00% | 13 | -13.33% | 0 |  | 1,251 | +2.04% |
| Render/expressions | 2 | -60.00% | 1 | -75.00% | 0 |  | 275 | +1.10% |
| Load/filters | 110 | -3.51% | 52 | -1.89% | 0 |  | 1,798 | -15.63% |
| Render/filters | 8 | -57.89% | 3 | -50.00% | 0 |  | 795 | +3.38% |
| Load/for_filter_if | 4 | 0% | 3 | 0% | 0 |  | 1,088 | +1.02% |
| Render/for_filter_if | 1,711 | -0.18% | 68 | +7.94% | 0 |  | 169 | +8.33% |
| Load/for_loop_vars | 23 | -11.54% | 16 | +14.29% | 0 |  | 1,090 | +1.58% |
| Render/for_loop_vars | 4 | 0% | 2 | 0% | 0 |  | 164 | +6.49% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 697 | -1.13% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 131 | +125.86% |
| Load/html_autoescape | 478 | +1.06% | 489 | -0.20% | 0 |  | 3,974 | -2.88% |
| Render/html_autoescape | 2,760 | -8.91% | 1,080 | +6.61% | 0 |  | 33,232 | -0.47% |
| Load/inheritance | 1 | 0% | 3 | +50.00% | 0 |  | 634 | +3.43% |
| Render/inheritance | 564 | +0.53% | 131 | +0.77% | 0 |  | 776 | +2.11% |
| Load/large_static | 1,943 | 0% | 887 | 0% | 0 |  | 262 | +3.97% |
| Render/large_static | 700 | 0% | 576 | 0% | 0 |  | 16 | +300.00% |
| Load/macros | 66 | -5.71% | 61 | +1.67% | 0 |  | 1,571 | -1.75% |
| Render/macros | 712 | 0% | 239 | -0.83% | 0 |  | 538 | -75.20% |
| Load/many_tags | 26,787 | +0.46% | 20,499 | +0.18% | 0 |  | 243,028 | -4.43% |
| Render/many_tags | 6,028 | +0.05% | 860 | +1.06% | 0 |  | 154 | +6.21% |
| Load/mitsuhiko_table | 18 | 0% | 28 | +21.74% | 0 |  | 801 | -5.09% |
| Render/mitsuhiko_table | 9,055 | +0.13% | 6,606 | -0.02% | 0 |  | 280 | -4.11% |
| Load/mitsuhiko_table_wide | 60 | -7.69% | 61 | -10.29% | 0 |  | 924 | -13.56% |
| Render/mitsuhiko_table_wide | 9,181 | -0.11% | 22,921 | +0.02% | 0 |  | 288 | +3.60% |
| Load/plain_text | 0 |  | 2 | -33.33% | 0 |  | 20 | +300.00% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 6 | +50.00% |
| Load/strings | 141 | -3.42% | 94 | -1.05% | 0 |  | 2,722 | -2.44% |
| Render/strings | 487 | +1.88% | 614 | 0% | 0 |  | 1,438 | +0.91% |
| Load/substitute | 1 | -50.00% | 3 | 0% | 0 |  | 132 | +0.76% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | +66.67% |

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
