# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (103 so far, latest [4f4e1ab](https://github.com/jinja2cpp/Jinja2Cpp/commit/4f4e1abb67414dfba1e6d3f9821c496a96038d34) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 361,615 | +0.03% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,723 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 416,968 | 0% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,763 | 0% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 278,496 | -0.02% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,134 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,621 | 0% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,281,881 | -0.05% |  | 1,046 | 0% |  | 23,784 | 0% | -0.10% |
| [Load/dict_ops](#loaddict_ops) | 34,016 | +0.03% | -76.98% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 304,768 | 0% | -55.88% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,848 | 0% | -77.76% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 450,179 | -0.04% | -50.32% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,727 | +0.06% | -73.14% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,737 | +0.17% | -64.21% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,721 | +0.01% | -77.31% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,230 | 0% | -68.55% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,325 | 0% | -77.13% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,926 | 0% | -81.64% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,900 | +0.01% | -75.48% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,883 | 0% | -57.83% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,324 | 0% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,230,344 | 0% |  | 734 | 0% |  | 43,760 | 0% | +8.36% |
| [Load/inheritance](#loadinheritance) | 23,590 | +0.02% | -79.33% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 133,674 | 0% | -82.13% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 157,570 | 0% | -82.22% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,037 | 0% | -72.00% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,746 | -0.07% | -75.11% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 646,182 | +0.03% | -74.79% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,301,797 | 0% | -78.94% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,156 | 0% | -64.75% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,966 | +0.02% | -79.81% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,147,598 | 0% | -58.34% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,678 | 0% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,492,838 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,720 | -0.03% | -61.56% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,016 | 0% | -85.05% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,004 | +0.09% | -74.64% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 615,121 | +0.07% | -70.67% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,863 | -0.01% | -73.66% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,244 | 0% | -74.54% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 739 | -1.73% | 731 | -0.41% | 0 |  | 8,057 | +5.21% |
| Render/chat_llama | 1,054 | +0.76% | 419 | +1.21% | 0 |  | 3,523 | +0.69% |
| Load/chat_mistral | 1,080 | -0.28% | 1,003 | 0% | 0 |  | 9,191 | +6.29% |
| Render/chat_mistral | 1,779 | -0.34% | 423 | +0.24% | 0 |  | 9,388 | +7.49% |
| Load/chat_qwen | 531 | +0.95% | 610 | +0.33% | 0 |  | 5,006 | +4.07% |
| Render/chat_qwen | 936 | +0.11% | 313 | 0% | 0 |  | 3,006 | -0.89% |
| Load/config_file | 184 | -3.16% | 186 | +0.54% | 0 |  | 3,079 | +3.99% |
| Render/config_file | 5,800 | -1.06% | 1,361 | -1.09% | 0 |  | 39,017 | +3.25% |
| Load/dict_ops | 23 | -20.69% | 14 | -22.22% | 0 |  | 1,295 | +4.69% |
| Render/dict_ops | 697 | -0.43% | 436 | +0.23% | 0 |  | 852 | -0.70% |
| Load/expressions | 36 | -5.26% | 29 | -3.33% | 0 |  | 1,283 | +2.64% |
| Render/expressions | 6 | 0% | 3 | 0% | 0 |  | 291 | +9.81% |
| Load/filters | 112 | -11.81% | 61 | -4.69% | 0 |  | 2,000 | +7.18% |
| Render/filters | 19 | 0% | 5 | 0% | 0 |  | 793 | -2.22% |
| Load/for_filter_if | 13 | +85.71% | 7 | +16.67% | 0 |  | 1,112 | +6.41% |
| Render/for_filter_if | 1,711 | 0% | 73 | +1.39% | 0 |  | 151 | -5.03% |
| Load/for_loop_vars | 34 | +6.25% | 24 | +4.35% | 0 |  | 1,130 | +5.12% |
| Render/for_loop_vars | 6 | 0% | 5 | 0% | 0 |  | 150 | +12.78% |
| Load/for_range | 2 | -33.33% | 5 | 0% | 0 |  | 716 | +1.85% |
| Render/for_range | 3 | 0% | 3 | 0% | 0 |  | 137 | +67.07% |
| Load/html_autoescape | 487 | -0.20% | 505 | +0.60% | 0 |  | 4,337 | +4.46% |
| Render/html_autoescape | 2,998 | -7.21% | 1,031 | -3.64% | 0 |  | 33,321 | -0.43% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 631 | +0.48% |
| Render/inheritance | 569 | +0.18% | 132 | +1.54% | 0 |  | 812 | +3.44% |
| Load/large_static | 1,948 | +0.15% | 895 | 0% | 0 |  | 249 | +4.62% |
| Render/large_static | 703 | 0% | 574 | 0% | 0 |  | 5 | +66.67% |
| Load/macros | 73 | +14.06% | 62 | +12.73% | 0 |  | 1,642 | +1.05% |
| Render/macros | 711 | 0% | 244 | 0% | 0 |  | 2,015 | -16.63% |
| Load/many_tags | 28,336 | -0.08% | 21,601 | -0.20% | 0 |  | 257,858 | +1.47% |
| Render/many_tags | 6,039 | +0.02% | 873 | -0.11% | 0 |  | 131 | -9.66% |
| Load/mitsuhiko_table | 26 | +36.84% | 31 | +6.90% | 0 |  | 804 | +2.16% |
| Render/mitsuhiko_table | 9,022 | 0% | 6,613 | -0.02% | 0 |  | 322 | +11.81% |
| Load/mitsuhiko_table_wide | 83 | -2.35% | 84 | 0% | 0 |  | 935 | -3.41% |
| Render/mitsuhiko_table_wide | 9,178 | -0.01% | 22,929 | +0.01% | 0 |  | 326 | +5.16% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | -54.55% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 6 | 0% |
| Load/strings | 164 | -4.65% | 114 | -0.87% | 0 |  | 2,772 | +2.59% |
| Render/strings | 468 | -1.68% | 617 | -0.16% | 0 |  | 1,447 | +1.47% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 122 | +35.56% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |

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
