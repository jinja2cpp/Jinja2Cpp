# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (107 so far, latest [66b9096](https://github.com/jinja2cpp/Jinja2Cpp/commit/66b90961af3e8ecfe75bea1b84fce9eddfec25a1) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,506 | -3.35% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 313,877 | -0.29% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 400,351 | -3.98% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 525,426 | -0.24% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 268,820 | -3.47% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 300,276 | -0.30% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,571 | -1.03% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,280,210 | -0.39% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 33,714 | -0.88% | -77.19% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 304,790 | 0% | -55.88% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,616 | -0.50% | -77.87% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 450,447 | +0.01% | -50.29% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,244 | -0.65% | -73.31% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,695 | -0.14% | -64.25% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,419 | -0.98% | -77.53% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,234 | 0% | -68.55% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,919 | -0.98% | -77.35% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,930 | 0% | -81.64% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,790 | -0.77% | -75.67% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,969 | +0.16% | -57.74% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,688 | -1.30% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,241,322 | +0.91% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,301 | -1.21% | -79.58% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 134,199 | -0.02% | -82.06% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 156,170 | -0.89% | -82.37% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,041 | 0% | -72.00% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,175 | -1.00% | -75.36% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 649,686 | 0% | -74.65% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,202,497 | -1.07% | -79.16% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,460 | 0% | -64.74% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,556 | -0.83% | -79.98% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,147,602 | 0% | -58.34% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,418 | -0.45% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,492,842 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,720 | -0.03% | -61.56% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,020 | 0% | -84.99% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,418 | -0.76% | -74.84% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 621,472 | +1.58% | -70.37% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,807 | -0.72% | -73.85% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,248 | 0% | -74.50% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 720 | -3.49% | 722 | -0.55% | 0 |  | 7,997 | +0.23% |
| Render/chat_llama | 1,050 | -0.38% | 417 | -0.71% | 0 |  | 3,239 | -6.25% |
| Load/chat_mistral | 1,064 | -1.66% | 1,011 | +0.60% | 0 |  | 8,915 | -0.22% |
| Render/chat_mistral | 1,803 | +3.86% | 426 | +1.91% | 0 |  | 8,628 | -2.14% |
| Load/chat_qwen | 509 | -3.23% | 605 | -1.14% | 0 |  | 4,842 | -2.42% |
| Render/chat_qwen | 967 | +0.94% | 311 | -1.89% | 0 |  | 2,846 | +1.93% |
| Load/config_file | 167 | -8.74% | 171 | -6.56% | 0 |  | 2,999 | -0.70% |
| Render/config_file | 5,900 | -0.17% | 1,418 | +1.07% | 0 |  | 38,099 | -0.41% |
| Load/dict_ops | 35 | +2.94% | 21 | -4.55% | 0 |  | 1,309 | +2.83% |
| Render/dict_ops | 688 | -1.01% | 433 | 0% | 0 |  | 845 | +2.80% |
| Load/expressions | 39 | +11.43% | 24 | -7.69% | 0 |  | 1,340 | +8.94% |
| Render/expressions | 6 | 0% | 4 | 0% | 0 |  | 300 | +11.52% |
| Load/filters | 108 | -3.57% | 63 | +1.61% | 0 |  | 2,070 | +8.83% |
| Render/filters | 37 | +117.65% | 7 | +40.00% | 0 |  | 785 | -0.88% |
| Load/for_filter_if | 16 | -5.88% | 8 | +14.29% | 0 |  | 1,130 | +3.48% |
| Render/for_filter_if | 1,717 | -0.46% | 74 | +5.71% | 0 |  | 148 | -17.78% |
| Load/for_loop_vars | 37 | +15.62% | 29 | +26.09% | 0 |  | 1,103 | 0% |
| Render/for_loop_vars | 14 | +100.00% | 7 | +16.67% | 0 |  | 139 | +13.01% |
| Load/for_range | 3 | 0% | 4 | -20.00% | 0 |  | 722 | +4.34% |
| Render/for_range | 4 | +100.00% | 2 |  | 0 |  | 101 | +34.67% |
| Load/html_autoescape | 478 | -1.24% | 501 | -1.57% | 0 |  | 4,202 | -1.98% |
| Render/html_autoescape | 3,605 | -9.22% | 1,092 | +2.82% | 0 |  | 33,404 | -0.56% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 602 | -3.99% |
| Render/inheritance | 574 | -0.86% | 133 | 0% | 0 |  | 835 | -0.71% |
| Load/large_static | 1,948 | -0.05% | 893 | -0.22% | 0 |  | 319 | +1.27% |
| Render/large_static | 704 | 0% | 574 | 0% | 0 |  | 3 | +50.00% |
| Load/macros | 76 | +7.04% | 55 | -5.17% | 0 |  | 1,664 | -0.42% |
| Render/macros | 713 | -0.14% | 244 | +0.83% | 0 |  | 532 | -4.32% |
| Load/many_tags | 27,813 | -2.20% | 21,461 | -0.68% | 0 |  | 254,348 | +2.25% |
| Render/many_tags | 6,038 | -0.08% | 879 | +0.80% | 0 |  | 138 | -4.83% |
| Load/mitsuhiko_table | 27 | -10.00% | 39 | 0% | 0 |  | 806 | -4.62% |
| Render/mitsuhiko_table | 9,038 | +0.03% | 6,623 | +0.08% | 0 |  | 297 | +4.21% |
| Load/mitsuhiko_table_wide | 76 | -5.00% | 73 | -1.35% | 0 |  | 1,145 | +19.64% |
| Render/mitsuhiko_table_wide | 9,256 | -0.06% | 22,938 | 0% | 0 |  | 292 | -1.02% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 13 | -13.33% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 6 | +50.00% |
| Load/strings | 171 | +6.88% | 112 | +5.66% | 0 |  | 2,797 | -0.46% |
| Render/strings | 479 | -0.62% | 616 | -0.48% | 0 |  | 1,424 | -2.53% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 153 | -3.16% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 2 | 0% |

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
