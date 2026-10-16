# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (89 so far, latest [13a4720](https://github.com/jinja2cpp/Jinja2Cpp/commit/13a47209d2033f0c891b9e249428485473f247ef) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,722 | 0% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 306,340 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,329 | 0% |  | 52 | 0% |  | 20,664 | 0% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 514,273 | 0% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 275,975 | 0% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 292,074 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,518 | 0% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,338,016 | 0% |  | 1,072 | 0% |  | 23,664 | 0% | -0.60% |
| [Load/dict_ops](#loaddict_ops) | 33,637 | 0% | -77.24% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 302,470 | 0% | -56.22% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,253 | 0% | -78.05% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,715 | 0% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,693 | 0% | -73.51% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,840 | -0.01% | -64.09% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,386 | 0% | -77.56% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 270,529 | 0% | -68.74% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,848 | 0% | -77.39% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,246 | 0% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,750 | 0% | -75.74% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,634 | 0% | -58.10% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,817 | 0% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,253,865 | 0% |  | 749 | 0% |  | 43,136 | 0% | +6.81% |
| [Load/inheritance](#loadinheritance) | 23,281 | 0% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 134,987 | 0% | -81.95% | 34 | 0% | -94.53% | 6,512 | 0% | +8.24% |
| [Load/large_static](#loadlarge_static) | 156,388 | 0% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,100 | 0% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,046 | 0% | -75.41% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 642,221 | 0% | -74.94% | 309 | 0% | -88.68% | 19,032 | 0% | +29.29% |
| [Load/many_tags](#loadmany_tags) | 9,188,937 | 0% | -79.19% | 945 | 0% | -97.24% | 421,312 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 728,830 | 0% | -65.01% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,459 | 0% | -80.02% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,117,004 | 0% | -58.59% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,502 | 0% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,462,367 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,695 | 0% | -61.81% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,001 | 0% | -85.27% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,987 | 0% | -74.99% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 600,220 | 0% | -71.38% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,790 | 0% | -73.91% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,228 | 0% | -74.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 671 | 0% | 686 | 0% | 0 |  | 7,185 | +5.54% |
| Render/chat_llama | 1,120 | 0% | 429 | 0% | 0 |  | 3,438 | +1.06% |
| Load/chat_mistral | 999 | 0% | 980 | 0% | 0 |  | 8,089 | +5.28% |
| Render/chat_mistral | 1,775 | 0% | 457 | 0% | 0 |  | 9,048 | -3.03% |
| Load/chat_qwen | 502 | 0% | 599 | 0% | 0 |  | 4,503 | +8.19% |
| Render/chat_qwen | 1,029 | 0% | 362 | 0% | 0 |  | 2,926 | +7.61% |
| Load/config_file | 182 | 0% | 164 | 0% | 0 |  | 2,881 | +1.12% |
| Render/config_file | 6,265 | 0% | 1,675 | 0% | 0 |  | 40,111 | -4.17% |
| Load/dict_ops | 38 | 0% | 23 | 0% | 0 |  | 1,265 | +2.43% |
| Render/dict_ops | 658 | 0% | 417 | 0% | 0 |  | 863 | +4.23% |
| Load/expressions | 36 | 0% | 34 | 0% | 0 |  | 1,250 | +3.22% |
| Render/expressions | 11 | 0% | 5 | 0% | 0 |  | 266 | -2.56% |
| Load/filters | 136 | 0% | 77 | 0% | 0 |  | 1,951 | +3.39% |
| Render/filters | 29 | 0% | 3 | 0% | 0 |  | 810 | +1.38% |
| Load/for_filter_if | 20 | 0% | 11 | 0% | 0 |  | 1,032 | -0.77% |
| Render/for_filter_if | 1,720 | 0% | 66 | 0% | 0 |  | 177 | +21.23% |
| Load/for_loop_vars | 46 | 0% | 26 | 0% | 0 |  | 1,053 | +1.25% |
| Render/for_loop_vars | 9 | 0% | 3 | 0% | 0 |  | 133 | +60.24% |
| Load/for_range | 13 | 0% | 12 | 0% | 0 |  | 728 | +3.41% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 118 | +1.72% |
| Load/html_autoescape | 418 | 0% | 502 | 0% | 0 |  | 3,960 | +2.51% |
| Render/html_autoescape | 4,848 | 0% | 1,480 | 0% | 0 |  | 36,601 | -1.16% |
| Load/inheritance | 9 | 0% | 7 | 0% | 0 |  | 617 | +3.35% |
| Render/inheritance | 588 | 0% | 125 | 0% | 0 |  | 722 | -2.56% |
| Load/large_static | 1,952 | 0% | 885 | 0% | 0 |  | 331 | +22.59% |
| Render/large_static | 707 | 0% | 571 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 74 | 0% | 52 | 0% | 0 |  | 1,604 | +0.82% |
| Render/macros | 760 | 0% | 240 | 0% | 0 |  | 1,167 | +12.10% |
| Load/many_tags | 27,712 | 0% | 20,660 | 0% | 0 |  | 236,053 | +2.23% |
| Render/many_tags | 6,057 | 0% | 867 | 0% | 0 |  | 132 | -2.22% |
| Load/mitsuhiko_table | 27 | 0% | 25 | 0% | 0 |  | 817 | +6.66% |
| Render/mitsuhiko_table | 9,281 | 0% | 6,609 | 0% | 0 |  | 285 | -4.04% |
| Load/mitsuhiko_table_wide | 84 | 0% | 116 | 0% | 0 |  | 1,274 | +1.92% |
| Render/mitsuhiko_table_wide | 9,398 | 0% | 22,906 | 0% | 0 |  | 283 | -1.39% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | +20.00% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 4 | +33.33% |
| Load/strings | 158 | 0% | 99 | 0% | 0 |  | 2,731 | +2.28% |
| Render/strings | 492 | 0% | 603 | 0% | 0 |  | 1,722 | +20.93% |
| Load/substitute | 3 | 0% | 4 | 0% | 0 |  | 143 | +17.21% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 4 | +33.33% |

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
