# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (108 so far, latest [9ec94e3](https://github.com/jinja2cpp/Jinja2Cpp/commit/9ec94e35a3ff0122de029a83c4c054f0093b322e) on 2026-10-18). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,605 | +0.03% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,871 | +0.32% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 400,377 | +0.01% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,358 | +0.18% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 268,828 | 0% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,119 | +0.28% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,622 | +0.05% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,290,421 | +0.80% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 33,714 | 0% | -77.19% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 307,522 | +0.90% | -55.49% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,609 | -0.02% | -77.88% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 451,062 | +0.14% | -50.22% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,343 | +0.14% | -73.28% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,733 | +0.12% | -64.21% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,411 | -0.03% | -77.54% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,666 | +0.16% | -68.50% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,919 | 0% | -77.35% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,445 | -0.25% | -81.69% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,790 | 0% | -75.67% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,284 | +0.81% | -57.40% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,646 | -0.02% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,242,196 | +0.07% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,301 | 0% | -79.58% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,476 | +1.70% | -81.75% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 156,170 | 0% | -82.37% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,051 | +0.05% | -71.98% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,175 | 0% | -75.36% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 649,401 | -0.04% | -74.66% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,212,097 | +0.10% | -79.14% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,470 | 0% | -64.74% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,556 | 0% | -79.98% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,185,599 | +0.74% | -58.03% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,418 | 0% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,530,843 | +0.69% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,720 | 0% | -61.56% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,030 | +0.98% | -84.84% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,491 | +0.10% | -74.82% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 623,659 | +0.35% | -70.27% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,807 | 0% | -73.85% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,258 | +0.44% | -74.39% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 734 | +1.94% | 727 | +0.69% | 0 |  | 7,549 | -5.60% |
| Render/chat_llama | 1,066 | +1.52% | 421 | +0.96% | 0 |  | 3,838 | +18.49% |
| Load/chat_mistral | 1,081 | +1.60% | 1,019 | +0.79% | 0 |  | 8,573 | -3.84% |
| Render/chat_mistral | 1,728 | -4.16% | 418 | -1.88% | 0 |  | 9,653 | +11.88% |
| Load/chat_qwen | 522 | +2.55% | 608 | +0.50% | 0 |  | 4,795 | -0.97% |
| Render/chat_qwen | 956 | -1.14% | 295 | -5.14% | 0 |  | 3,039 | +6.78% |
| Load/config_file | 160 | -4.19% | 163 | -4.68% | 0 |  | 2,994 | -0.17% |
| Render/config_file | 6,385 | +8.22% | 1,920 | +35.40% | 0 |  | 38,783 | +1.80% |
| Load/dict_ops | 31 | -11.43% | 21 | 0% | 0 |  | 1,309 | 0% |
| Render/dict_ops | 680 | -1.16% | 437 | +0.92% | 0 |  | 848 | +0.36% |
| Load/expressions | 33 | -15.38% | 23 | -4.17% | 0 |  | 1,360 | +1.49% |
| Render/expressions | 8 | +33.33% | 5 | +25.00% | 0 |  | 311 | +3.67% |
| Load/filters | 104 | -3.70% | 58 | -7.94% | 0 |  | 2,172 | +4.93% |
| Render/filters | 17 | -54.05% | 5 | -28.57% | 0 |  | 769 | -2.04% |
| Load/for_filter_if | 12 | -25.00% | 8 | 0% | 0 |  | 1,116 | -1.24% |
| Render/for_filter_if | 1,716 | -0.06% | 70 | -5.41% | 0 |  | 144 | -2.70% |
| Load/for_loop_vars | 35 | -5.41% | 25 | -13.79% | 0 |  | 1,132 | +2.63% |
| Render/for_loop_vars | 11 | -21.43% | 6 | -14.29% | 0 |  | 137 | -1.44% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 741 | +2.63% |
| Render/for_range | 11 | +175.00% | 5 | +150.00% | 0 |  | 154 | +52.48% |
| Load/html_autoescape | 470 | -1.67% | 503 | +0.40% | 0 |  | 4,417 | +5.12% |
| Render/html_autoescape | 3,877 | +7.55% | 1,312 | +20.15% | 0 |  | 34,223 | +2.45% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 621 | +3.16% |
| Render/inheritance | 570 | -0.70% | 125 | -6.02% | 0 |  | 822 | -1.56% |
| Load/large_static | 1,947 | -0.05% | 893 | 0% | 0 |  | 356 | +11.60% |
| Render/large_static | 704 | 0% | 574 | 0% | 0 |  | 5 | +66.67% |
| Load/macros | 75 | -1.32% | 54 | -1.82% | 0 |  | 1,686 | +1.32% |
| Render/macros | 729 | +2.24% | 249 | +2.05% | 0 |  | 545 | +2.44% |
| Load/many_tags | 27,679 | -0.48% | 21,474 | +0.06% | 0 |  | 259,343 | +1.96% |
| Render/many_tags | 6,038 | 0% | 873 | -0.68% | 0 |  | 191 | +38.41% |
| Load/mitsuhiko_table | 25 | -7.41% | 34 | -12.82% | 0 |  | 828 | +2.73% |
| Render/mitsuhiko_table | 9,042 | +0.04% | 6,618 | -0.08% | 0 |  | 306 | +3.03% |
| Load/mitsuhiko_table_wide | 74 | -2.63% | 75 | +2.74% | 0 |  | 903 | -21.14% |
| Render/mitsuhiko_table_wide | 9,232 | -0.26% | 22,946 | +0.03% | 0 |  | 286 | -2.05% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 13 | 0% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 5 | -16.67% |
| Load/strings | 164 | -4.09% | 106 | -5.36% | 0 |  | 2,842 | +1.61% |
| Render/strings | 472 | -1.46% | 614 | -0.32% | 0 |  | 1,759 | +23.53% |
| Load/substitute | 3 | +50.00% | 4 | 0% | 0 |  | 214 | +39.87% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 4 | +100.00% |

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
