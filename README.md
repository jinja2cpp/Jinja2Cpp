# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (63 so far, latest [2a844f9](https://github.com/jinja2cpp/Jinja2Cpp/commit/2a844f9dc299defd0d93a413fb8b8103242633ac) on 2026-10-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 555,170 | +0.03% |  | 92 | 0% |  | 19,856 | 0% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 406,909 | 0% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 657,629 | +0.05% |  | 75 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 654,427 | 0% |  | 426 | 0% |  | 10,392 | 0% | -21.32% |
| [Load/chat_qwen](#loadchat_qwen) | 424,516 | +0.03% |  | 68 | 0% |  | 15,760 | 0% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 393,717 | 0% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 145,958 | +0.09% |  | 34 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,735,583 | 0% |  | 1,169 | 0% |  | 21,656 | 0% | -9.04% |
| [Load/dict_ops](#loaddict_ops) | 47,706 | +0.06% | -67.72% | 12 | 0% | -90.84% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 351,754 | 0% | -49.08% | 112 | 0% | -75.11% | 35,608 | 0% | +2.63% |
| [Load/expressions](#loadexpressions) | 67,192 | +0.04% | -67.41% | 18 | 0% | -87.67% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 517,674 | 0% | -42.87% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,655 | +0.02% | -64.42% | 32 | 0% | -85.96% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,754 | 0% | -40.50% | 35 | 0% | -52.70% | 1,992 | 0% | -59.45% |
| [Load/for_filter_if](#loadfor_filter_if) | 41,106 | +0.07% | -69.64% | 9 | 0% | -92.68% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 447,593 | 0% | -48.28% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 57,243 | +0.06% | -68.31% | 13 | 0% | -91.72% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,241 | 0% | -74.62% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 19,594 | +0.12% | -65.43% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,430 | 0% | -49.65% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 289,624 | +0.02% |  | 61 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,599,247 | 0% |  | 1,003 | 0% |  | 38,816 | 0% | -3.88% |
| [Load/inheritance](#loadinheritance) | 34,687 | +0.09% | -69.60% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 193,529 | -0.01% | -74.12% | 73 | 0% | -88.26% | 3,296 | 0% | -45.21% |
| [Load/large_static](#loadlarge_static) | 237,668 | +0.02% | -73.17% | 28 | 0% | -93.64% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,439 | 0% | -71.47% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,663 | +0.03% | -64.18% | 29 | 0% | -84.41% | 3,728 | 0% | -40.03% |
| [Render/macros](#rendermacros) | 1,007,488 | 0% | -60.69% | 406 | 0% | -85.12% | 13,608 | 0% | -7.55% |
| [Load/many_tags](#loadmany_tags) | 13,310,876 | +0.03% | -69.86% | 962 | 0% | -97.19% | 445,680 | 0% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,165,611 | +0.01% | -44.04% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 78,111 | +0.04% | -67.12% | 26 | 0% | -85.87% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,623,784 | 0% | -46.39% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 96,464 | -0.13% |  | 57 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,994,172 | 0% |  | 1,043 | 0% |  | 1,379,792 | 0% | -0.09% |
| [Load/plain_text](#loadplain_text) | 4,068 | +0.47% | -57.96% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 101,400 | +0.03% | -65.26% | 33 | 0% | -85.27% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,055,144 | 0% | -49.70% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,865 | +0.19% | -63.61% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,662 | 0% | -69.80% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,188 | +0.25% | 1,385 | +0.07% | 0 |  | 9,008 | -0.49% |
| Render/chat_llama | 1,275 | 0% | 508 | 0% | 0 |  | 4,594 | -5.26% |
| Load/chat_mistral | 1,814 | +0.83% | 2,066 | -0.10% | 0 |  | 10,485 | -1.64% |
| Render/chat_mistral | 2,176 | 0% | 538 | 0% | 0 |  | 13,341 | -5.40% |
| Load/chat_qwen | 836 | +0.36% | 1,240 | -0.08% | 0 |  | 5,966 | -1.68% |
| Render/chat_qwen | 1,209 | 0% | 380 | 0% | 0 |  | 4,353 | -0.48% |
| Load/config_file | 380 | +1.06% | 562 | +0.90% | 0 |  | 3,602 | -2.78% |
| Render/config_file | 7,052 | -1.07% | 2,046 | -0.53% | 0 |  | 68,849 | -0.89% |
| Load/dict_ops | 106 | 0% | 109 | +1.87% | 0 |  | 1,583 | -2.28% |
| Render/dict_ops | 949 | 0% | 728 | 0% | 0 |  | 1,108 | +2.97% |
| Load/expressions | 130 | -1.52% | 204 | 0% | 0 |  | 1,532 | +0.26% |
| Render/expressions | 10 | 0% | 3 | 0% | 0 |  | 479 | -1.03% |
| Load/filters | 208 | -0.48% | 151 | 0% | 0 |  | 2,631 | -4.33% |
| Render/filters | 56 | 0% | 30 | 0% | 0 |  | 1,135 | -0.26% |
| Load/for_filter_if | 53 | +1.92% | 42 | +5.00% | 0 |  | 1,223 | -2.24% |
| Render/for_filter_if | 1,310 | 0% | 58 | 0% | 0 |  | 405 | -5.81% |
| Load/for_loop_vars | 55 | +7.84% | 75 | -1.32% | 0 |  | 1,375 | -4.91% |
| Render/for_loop_vars | 9 | 0% | 6 | 0% | 0 |  | 270 | -8.78% |
| Load/for_range | 21 | +5.00% | 26 | +8.33% | 0 |  | 961 | +2.13% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 233 | +1.75% |
| Load/html_autoescape | 654 | +1.40% | 880 | +0.23% | 0 |  | 5,045 | -8.52% |
| Render/html_autoescape | 4,170 | -0.10% | 1,485 | +0.07% | 0 |  | 49,509 | -2.57% |
| Load/inheritance | 25 | -3.85% | 23 | -8.00% | 0 |  | 951 | +1.82% |
| Render/inheritance | 505 | 0% | 101 | +2.02% | 0 |  | 2,004 | +153.99% |
| Load/large_static | 2,619 | +0.04% | 1,190 | 0% | 0 |  | 517 | -11.93% |
| Render/large_static | 712 | 0% | 579 | 0% | 0 |  | 11 | +175.00% |
| Load/macros | 214 | 0% | 274 | +1.48% | 0 |  | 2,054 | -0.82% |
| Render/macros | 666 | 0% | 208 | 0% | 0 |  | 9,884 | -10.97% |
| Load/many_tags | 40,152 | -0.04% | 33,258 | -0.01% | 0 |  | 316,165 | -4.90% |
| Render/many_tags | 6,877 | +0.17% | 1,463 | -0.61% | 0 |  | 565 | -84.78% |
| Load/mitsuhiko_table | 121 | +4.31% | 205 | +0.99% | 0 |  | 1,294 | +0.54% |
| Render/mitsuhiko_table | 8,021 | 0% | 5,014 | 0% | 0 |  | 480 | -94.91% |
| Load/mitsuhiko_table_wide | 198 | -0.50% | 335 | +0.60% | 0 |  | 1,580 | -4.82% |
| Render/mitsuhiko_table_wide | 8,077 | 0% | 21,460 | 0% | 0 |  | 490 | -4.30% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 20 | -4.76% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| Load/strings | 256 | 0% | 282 | 0% | 0 |  | 3,236 | -1.34% |
| Render/strings | 913 | 0% | 1,349 | 0% | 0 |  | 2,658 | +33.63% |
| Load/substitute | 7 | 0% | 12 | 0% | 0 |  | 350 | -14.22% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 3 | 0% |

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
