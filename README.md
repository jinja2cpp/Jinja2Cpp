# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (111 so far, latest [7c838c0](https://github.com/jinja2cpp/Jinja2Cpp/commit/7c838c06638dbfee9e12f416b3ee297d46521cd1) on 2026-10-18). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,506 | -0.03% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,882 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 400,317 | -0.01% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,358 | 0% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 268,826 | 0% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,119 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,636 | +0.01% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,290,962 | +0.04% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 33,714 | 0% | -77.19% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 307,522 | 0% | -55.49% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,609 | 0% | -77.88% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 451,062 | 0% | -50.22% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,343 | 0% | -73.28% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,737 | +0.01% | -64.21% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,419 | +0.03% | -77.53% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,666 | 0% | -68.50% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,891 | -0.07% | -77.37% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,445 | 0% | -81.69% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,790 | 0% | -75.67% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,284 | 0% | -57.40% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,674 | +0.01% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,242,196 | 0% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,301 | 0% | -79.58% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,476 | 0% | -81.75% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 156,170 | 0% | -82.37% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,051 | 0% | -71.98% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,175 | 0% | -75.36% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 649,401 | 0% | -74.66% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,212,097 | 0% | -79.14% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,470 | 0% | -64.74% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,556 | 0% | -79.98% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,185,610 | 0% | -58.03% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,418 | 0% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,530,854 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,720 | 0% | -61.56% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,030 | 0% | -84.84% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,491 | 0% | -74.82% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 623,659 | 0% | -70.27% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,807 | 0% | -73.85% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,258 | 0% | -74.39% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 725 | -1.23% | 722 | -0.69% | 0 |  | 7,545 | -0.03% |
| Render/chat_llama | 1,061 | -0.47% | 421 | 0% | 0 |  | 3,673 | -4.30% |
| Load/chat_mistral | 1,061 | -1.85% | 1,013 | -0.59% | 0 |  | 8,494 | -0.94% |
| Render/chat_mistral | 1,745 | +0.98% | 420 | +0.48% | 0 |  | 9,107 | -5.75% |
| Load/chat_qwen | 497 | -4.79% | 600 | -1.32% | 0 |  | 4,623 | -3.55% |
| Render/chat_qwen | 942 | -1.46% | 296 | +0.34% | 0 |  | 2,887 | -5.00% |
| Load/config_file | 161 | +0.63% | 169 | +3.68% | 0 |  | 2,994 | +0.07% |
| Render/config_file | 6,457 | +1.13% | 1,939 | +0.99% | 0 |  | 37,879 | -2.33% |
| Load/dict_ops | 33 | +6.45% | 22 | +4.76% | 0 |  | 1,286 | -1.53% |
| Render/dict_ops | 672 | -1.18% | 436 | -0.23% | 0 |  | 834 | -1.65% |
| Load/expressions | 30 | -9.09% | 19 | -17.39% | 0 |  | 1,301 | -3.91% |
| Render/expressions | 7 | -12.50% | 5 | 0% | 0 |  | 316 | +1.61% |
| Load/filters | 106 | +1.92% | 61 | +5.17% | 0 |  | 2,264 | +4.19% |
| Render/filters | 16 | -5.88% | 4 | -20.00% | 0 |  | 796 | +3.51% |
| Load/for_filter_if | 12 | 0% | 7 | -12.50% | 0 |  | 1,081 | -2.88% |
| Render/for_filter_if | 1,715 | -0.06% | 70 | 0% | 0 |  | 134 | -6.94% |
| Load/for_loop_vars | 28 | -20.00% | 24 | -4.00% | 0 |  | 1,110 | -1.51% |
| Render/for_loop_vars | 8 | -27.27% | 6 | 0% | 0 |  | 134 | -2.19% |
| Load/for_range | 3 | 0% | 4 | 0% | 0 |  | 700 | -4.63% |
| Render/for_range | 7 | -36.36% | 4 | -20.00% | 0 |  | 147 | -4.55% |
| Load/html_autoescape | 462 | -1.70% | 502 | -0.20% | 0 |  | 4,350 | -1.52% |
| Render/html_autoescape | 4,311 | +11.19% | 1,352 | +3.05% | 0 |  | 32,898 | -4.56% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 645 | +3.70% |
| Render/inheritance | 571 | +0.18% | 125 | 0% | 0 |  | 846 | +2.79% |
| Load/large_static | 1,948 | +0.05% | 893 | 0% | 0 |  | 291 | -17.56% |
| Render/large_static | 704 | 0% | 574 | 0% | 0 |  | 4 | -20.00% |
| Load/macros | 72 | -4.00% | 55 | +1.85% | 0 |  | 1,642 | -2.61% |
| Render/macros | 729 | 0% | 251 | +0.80% | 0 |  | 1,527 | +180.18% |
| Load/many_tags | 27,598 | -0.29% | 21,440 | -0.16% | 0 |  | 256,671 | -1.03% |
| Render/many_tags | 6,036 | -0.03% | 876 | +0.34% | 0 |  | 126 | -34.03% |
| Load/mitsuhiko_table | 29 | +16.00% | 35 | +2.94% | 0 |  | 857 | +3.88% |
| Render/mitsuhiko_table | 9,033 | -0.10% | 6,614 | -0.06% | 0 |  | 318 | +3.92% |
| Load/mitsuhiko_table_wide | 69 | -6.76% | 70 | -6.67% | 0 |  | 937 | +3.42% |
| Render/mitsuhiko_table_wide | 9,227 | -0.05% | 22,936 | -0.04% | 0 |  | 282 | -1.40% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 14 | -6.67% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 7 | +40.00% |
| Load/strings | 161 | -1.83% | 110 | +3.77% | 0 |  | 2,832 | -0.35% |
| Render/strings | 471 | -0.21% | 614 | 0% | 0 |  | 1,452 | -17.45% |
| Load/substitute | 2 | -33.33% | 4 | 0% | 0 |  | 187 | -10.10% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 4 | 0% |

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
