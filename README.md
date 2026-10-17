# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (105 so far, latest [498c671](https://github.com/jinja2cpp/Jinja2Cpp/commit/498c671cf509c9ffca4a2a4e636d673247bd32de) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 361,611 | 0% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,785 | +0.02% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 416,962 | 0% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,665 | -0.02% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 278,488 | 0% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,184 | +0.02% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,618 | 0% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,281,881 | 0% |  | 1,046 | 0% |  | 23,784 | 0% | -0.10% |
| [Load/dict_ops](#loaddict_ops) | 34,014 | 0% | -76.98% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 304,786 | +0.01% | -55.88% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,848 | 0% | -77.76% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 450,379 | +0.04% | -50.30% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,727 | 0% | -73.14% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,737 | 0% | -64.21% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,718 | 0% | -77.31% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,230 | 0% | -68.55% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,324 | 0% | -77.13% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,926 | 0% | -81.64% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,899 | 0% | -75.48% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,901 | +0.05% | -57.81% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,307 | -0.01% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,228,904 | -0.12% |  | 734 | 0% |  | 43,760 | 0% | +8.36% |
| [Load/inheritance](#loadinheritance) | 23,589 | 0% | -79.33% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 133,674 | 0% | -82.13% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 157,570 | 0% | -82.22% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,037 | 0% | -72.00% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,745 | 0% | -75.11% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 646,282 | +0.02% | -74.78% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,301,797 | 0% | -78.94% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,456 | +0.04% | -64.74% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,963 | 0% | -79.81% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,147,598 | 0% | -58.34% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,675 | 0% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,492,838 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,720 | 0% | -61.56% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,016 | 0% | -85.05% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,988 | -0.02% | -74.65% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 611,773 | -0.54% | -70.83% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,863 | 0% | -73.66% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,244 | 0% | -74.54% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 751 | +1.62% | 729 | -0.27% | 0 |  | 7,871 | -3.34% |
| Render/chat_llama | 1,047 | -0.66% | 420 | +0.24% | 0 |  | 3,375 | -3.68% |
| Load/chat_mistral | 1,094 | +1.30% | 1,004 | +0.10% | 0 |  | 8,691 | -5.48% |
| Render/chat_mistral | 1,776 | -0.17% | 418 | -1.18% | 0 |  | 9,301 | +1.57% |
| Load/chat_qwen | 524 | -1.32% | 608 | -0.33% | 0 |  | 4,431 | -12.67% |
| Render/chat_qwen | 959 | +2.46% | 324 | +3.51% | 0 |  | 2,863 | +0.10% |
| Load/config_file | 195 | +5.98% | 189 | +1.61% | 0 |  | 2,989 | -2.42% |
| Render/config_file | 5,778 | -0.38% | 1,367 | +0.44% | 0 |  | 40,243 | +8.21% |
| Load/dict_ops | 36 | +56.52% | 19 | +35.71% | 0 |  | 1,277 | -3.33% |
| Render/dict_ops | 700 | +0.43% | 436 | 0% | 0 |  | 879 | +5.14% |
| Load/expressions | 42 | +16.67% | 29 | 0% | 0 |  | 1,289 | -2.35% |
| Render/expressions | 5 | -16.67% | 3 | 0% | 0 |  | 323 | +10.62% |
| Load/filters | 119 | +6.25% | 58 | -4.92% | 0 |  | 1,988 | -0.55% |
| Render/filters | 16 | -15.79% | 5 | 0% | 0 |  | 826 | +3.38% |
| Load/for_filter_if | 14 | +7.69% | 7 | 0% | 0 |  | 1,078 | -4.85% |
| Render/for_filter_if | 1,715 | +0.23% | 72 | -1.37% | 0 |  | 179 | +31.62% |
| Load/for_loop_vars | 37 | +8.82% | 24 | 0% | 0 |  | 1,072 | -6.05% |
| Render/for_loop_vars | 5 | -16.67% | 4 | -20.00% | 0 |  | 124 | -6.06% |
| Load/for_range | 3 | +50.00% | 5 | 0% | 0 |  | 736 | +1.38% |
| Render/for_range | 3 | 0% | 2 | -33.33% | 0 |  | 119 | +16.67% |
| Load/html_autoescape | 479 | -1.64% | 506 | +0.20% | 0 |  | 4,080 | -6.31% |
| Render/html_autoescape | 3,392 | +13.14% | 1,049 | +1.75% | 0 |  | 31,174 | -10.48% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 624 | -1.73% |
| Render/inheritance | 565 | -0.70% | 132 | 0% | 0 |  | 796 | -3.63% |
| Load/large_static | 1,948 | 0% | 895 | 0% | 0 |  | 252 | +2.86% |
| Render/large_static | 703 | 0% | 574 | 0% | 0 |  | 11 | +175.00% |
| Load/macros | 68 | -6.85% | 57 | -8.06% | 0 |  | 1,662 | -1.60% |
| Render/macros | 711 | 0% | 243 | -0.41% | 0 |  | 511 | -4.31% |
| Load/many_tags | 28,965 | +2.22% | 21,714 | +0.52% | 0 |  | 261,850 | -0.76% |
| Render/many_tags | 6,039 | 0% | 872 | -0.11% | 0 |  | 124 | +0.81% |
| Load/mitsuhiko_table | 29 | +11.54% | 37 | +19.35% | 0 |  | 838 | +0.96% |
| Render/mitsuhiko_table | 9,022 | 0% | 6,615 | +0.03% | 0 |  | 302 | +1.34% |
| Load/mitsuhiko_table_wide | 79 | -4.82% | 80 | -4.76% | 0 |  | 1,055 | +10.82% |
| Render/mitsuhiko_table_wide | 9,189 | +0.12% | 22,923 | -0.03% | 0 |  | 306 | -7.55% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 6 | 0% |
| Load/strings | 162 | -1.22% | 105 | -7.89% | 0 |  | 2,836 | +1.47% |
| Render/strings | 474 | +1.28% | 618 | +0.16% | 0 |  | 1,437 | +0.49% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 103 | -25.90% |
| Render/substitute | 0 |  | 1 |  | 0 |  | 4 | +100.00% |

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
