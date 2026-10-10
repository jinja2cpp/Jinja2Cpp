# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (62 so far, latest [0aa521d](https://github.com/jinja2cpp/Jinja2Cpp/commit/0aa521d1b43fd17e78f98c19ccf13a46abeb61d6) on 2026-10-10). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 555,024 | +0.18% |  | 92 | 0% |  | 19,856 | -1.35% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 406,909 | -0.41% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 657,305 | +0.56% |  | 75 | +1.35% |  | 23,832 | -1.52% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 654,427 | -0.34% |  | 426 | 0% |  | 10,392 | -0.46% | -21.32% |
| [Load/chat_qwen](#loadchat_qwen) | 424,389 | +0.33% |  | 68 | 0% |  | 15,760 | -1.50% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 393,717 | -0.67% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 145,821 | +0.42% |  | 34 | 0% |  | 5,576 | -1.41% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,735,619 | -3.67% |  | 1,169 | 0% |  | 21,656 | 0% | -9.04% |
| [Load/dict_ops](#loaddict_ops) | 47,677 | +0.56% | -67.74% | 12 | 0% | -90.84% | 2,224 | -0.71% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 351,754 | -0.04% | -49.08% | 112 | 0% | -75.11% | 35,608 | 0% | +2.63% |
| [Load/expressions](#loadexpressions) | 67,164 | +0.34% | -67.42% | 18 | 0% | -87.67% | 3,560 | -0.89% | -41.98% |
| [Render/expressions](#renderexpressions) | 517,674 | -0.18% | -42.87% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,632 | +0.38% | -64.42% | 32 | 0% | -85.96% | 3,896 | -1.62% | -65.87% |
| [Render/filters](#renderfilters) | 52,754 | -1.66% | -40.50% | 35 | 0% | -52.70% | 1,992 | 0% | -59.45% |
| [Load/for_filter_if](#loadfor_filter_if) | 41,077 | +0.44% | -69.66% | 9 | 0% | -92.68% | 1,936 | -0.82% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 447,593 | -0.09% | -48.28% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 57,211 | +0.53% | -68.33% | 13 | 0% | -91.72% | 2,416 | -1.31% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,241 | -0.22% | -74.62% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 19,570 | +0.41% | -65.48% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,430 | -0.23% | -49.65% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 289,561 | +0.86% |  | 61 | 0% |  | 12,944 | -1.58% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,599,256 | -0.10% |  | 1,003 | 0% |  | 38,816 | 0% | -3.88% |
| [Load/inheritance](#loadinheritance) | 34,656 | -0.30% | -69.63% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 193,547 | -9.56% | -74.12% | 73 | 0% | -88.26% | 3,296 | 0% | -45.21% |
| [Load/large_static](#loadlarge_static) | 237,621 | +0.32% | -73.18% | 28 | 0% | -93.64% | 42,720 | -0.93% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,439 | -22.10% | -71.47% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,636 | +0.77% | -64.19% | 29 | 0% | -84.41% | 3,728 | -0.43% | -40.03% |
| [Render/macros](#rendermacros) | 1,007,488 | -0.11% | -60.69% | 406 | 0% | -85.12% | 13,608 | 0% | -7.55% |
| [Load/many_tags](#loadmany_tags) | 13,307,548 | +0.69% | -69.87% | 962 | +0.42% | -97.19% | 445,680 | -2.66% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,165,498 | -7.18% | -44.05% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 78,081 | +0.51% | -67.13% | 26 | 0% | -85.87% | 3,552 | -0.45% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,623,784 | -0.17% | -46.39% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 96,588 | +0.40% |  | 57 | 0% |  | 5,736 | -0.28% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,994,172 | -0.16% |  | 1,043 | 0% |  | 1,379,792 | 0% | -0.09% |
| [Load/plain_text](#loadplain_text) | 4,049 | +0.12% | -58.16% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 101,367 | +0.37% | -65.27% | 33 | 0% | -85.27% | 4,504 | -1.05% | -50.18% |
| [Render/strings](#renderstrings) | 1,055,144 | -0.02% | -49.70% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,844 | +1.33% | -63.68% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,662 | -0.37% | -69.80% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,185 | -3.42% | 1,384 | -1.07% | 0 |  | 9,052 | +9.13% |
| Render/chat_llama | 1,275 | -0.16% | 508 | -0.59% | 0 |  | 4,849 | +1.81% |
| Load/chat_mistral | 1,799 | -0.39% | 2,068 | +2.38% | 0 |  | 10,660 | +10.83% |
| Render/chat_mistral | 2,176 | -1.54% | 538 | -1.10% | 0 |  | 14,102 | +1.13% |
| Load/chat_qwen | 833 | -4.03% | 1,241 | -2.51% | 0 |  | 6,068 | +14.60% |
| Render/chat_qwen | 1,209 | +0.50% | 380 | 0% | 0 |  | 4,374 | -1.09% |
| Load/config_file | 376 | -1.31% | 557 | +2.58% | 0 |  | 3,705 | +8.30% |
| Render/config_file | 7,128 | -2.86% | 2,057 | +4.79% | 0 |  | 69,464 | -0.86% |
| Load/dict_ops | 106 | +8.16% | 107 | +16.30% | 0 |  | 1,620 | +6.58% |
| Render/dict_ops | 949 | -0.63% | 728 | 0% | 0 |  | 1,076 | -0.09% |
| Load/expressions | 132 | -7.69% | 204 | -2.39% | 0 |  | 1,528 | +5.16% |
| Render/expressions | 10 | 0% | 3 | 0% | 0 |  | 484 | +1.26% |
| Load/filters | 209 | -2.34% | 151 | -1.95% | 0 |  | 2,750 | +16.48% |
| Render/filters | 56 | -6.67% | 30 | -18.92% | 0 |  | 1,138 | +1.79% |
| Load/for_filter_if | 52 | +8.33% | 40 | +48.15% | 0 |  | 1,251 | +6.11% |
| Render/for_filter_if | 1,310 | +0.15% | 58 | +1.75% | 0 |  | 430 | +5.91% |
| Load/for_loop_vars | 51 | -13.56% | 76 | +10.14% | 0 |  | 1,446 | +9.38% |
| Render/for_loop_vars | 9 | -40.00% | 6 | -40.00% | 0 |  | 296 | +11.28% |
| Load/for_range | 20 | -16.67% | 24 | 0% | 0 |  | 941 | +1.73% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 229 | -6.53% |
| Load/html_autoescape | 645 | -10.66% | 878 | -1.35% | 0 |  | 5,515 | +19.76% |
| Render/html_autoescape | 4,174 | -3.65% | 1,484 | -5.36% | 0 |  | 50,816 | +1.97% |
| Load/inheritance | 26 | +44.44% | 25 | +8.70% | 0 |  | 934 | +1.41% |
| Render/inheritance | 505 | -1.56% | 99 | +1.02% | 0 |  | 789 | -2.59% |
| Load/large_static | 2,618 | -0.08% | 1,190 | -0.67% | 0 |  | 587 | +38.12% |
| Render/large_static | 712 | -1.93% | 579 | -0.52% | 0 |  | 4 | 0% |
| Load/macros | 214 | +3.38% | 270 | +3.85% | 0 |  | 2,071 | +6.92% |
| Render/macros | 666 | -0.15% | 208 | +0.48% | 0 |  | 11,102 | +15.89% |
| Load/many_tags | 40,167 | +6.23% | 33,261 | +9.18% | 0 |  | 332,461 | +16.29% |
| Render/many_tags | 6,865 | -5.57% | 1,472 | -15.26% | 0 |  | 3,713 | +566.61% |
| Load/mitsuhiko_table | 116 | +13.73% | 203 | +6.84% | 0 |  | 1,287 | +3.62% |
| Render/mitsuhiko_table | 8,021 | +0.01% | 5,014 | +0.02% | 0 |  | 9,423 | +26.33% |
| Load/mitsuhiko_table_wide | 199 | -2.45% | 333 | +3.10% | 0 |  | 1,660 | +1.90% |
| Render/mitsuhiko_table_wide | 8,077 | -0.10% | 21,460 | -0.01% | 0 |  | 512 | -93.15% |
| Load/plain_text | 0 |  | 3 | +50.00% | 0 |  | 21 | +75.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 256 | +0.39% | 282 | -0.70% | 0 |  | 3,280 | +6.46% |
| Render/strings | 913 | -1.83% | 1,349 | +0.75% | 0 |  | 1,989 | -25.11% |
| Load/substitute | 7 | 0% | 12 | +9.09% | 0 |  | 408 | +15.91% |
| Render/substitute | 0 |  | 1 |  | 0 |  | 3 | -25.00% |

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
