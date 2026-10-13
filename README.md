# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (88 so far, latest [a69c07b](https://github.com/jinja2cpp/Jinja2Cpp/commit/a69c07b74cc08f5145adc72a5f4d6c2c84a97333) on 2026-10-13). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,722 | -0.01% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 306,343 | -0.67% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,329 | -0.02% |  | 52 | 0% |  | 20,664 | 0% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 514,276 | -0.29% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 275,975 | -0.03% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 292,074 | -0.53% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,518 | +0.02% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,338,016 | +0.04% |  | 1,072 | 0% |  | 23,664 | 0% | -0.60% |
| [Load/dict_ops](#loaddict_ops) | 33,637 | +0.04% | -77.24% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 302,470 | 0% | -56.22% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,253 | 0% | -78.05% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,715 | 0% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,693 | +0.01% | -73.51% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 31,842 | +0.82% | -64.09% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,386 | +0.07% | -77.56% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 270,529 | 0% | -68.74% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,848 | +0.02% | -77.39% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,246 | 0% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,750 | 0% | -75.74% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,634 | 0% | -58.10% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,817 | -0.03% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,253,865 | -0.33% |  | 749 | 0% |  | 43,136 | 0% | +6.81% |
| [Load/inheritance](#loadinheritance) | 23,281 | 0% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 134,987 | 0% | -81.95% | 34 | 0% | -94.53% | 6,512 | 0% | +8.24% |
| [Load/large_static](#loadlarge_static) | 156,388 | 0% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,100 | 0% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,046 | 0% | -75.41% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 642,221 | 0% | -74.94% | 309 | 0% | -88.68% | 19,032 | 0% | +29.29% |
| [Load/many_tags](#loadmany_tags) | 9,188,937 | +0.05% | -79.19% | 945 | 0% | -97.24% | 421,312 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 728,830 | +1.50% | -65.01% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,459 | 0% | -80.02% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,117,004 | 0% | -58.59% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,502 | 0% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,462,367 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,695 | 0% | -61.81% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,001 | 0% | -85.27% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,987 | -0.16% | -74.99% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 600,223 | +0.35% | -71.38% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,790 | 0% | -73.91% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,228 | 0% | -74.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 671 | +0.45% | 686 | -0.58% | 0 |  | 6,808 | -7.54% |
| Render/chat_llama | 1,120 | +0.72% | 429 | +0.94% | 0 |  | 3,402 | +10.38% |
| Load/chat_mistral | 999 | +2.04% | 980 | +1.24% | 0 |  | 7,683 | -5.60% |
| Render/chat_mistral | 1,775 | +2.07% | 457 | +3.39% | 0 |  | 9,331 | +4.80% |
| Load/chat_qwen | 502 | -1.57% | 599 | -0.50% | 0 |  | 4,162 | -7.55% |
| Render/chat_qwen | 1,029 | -2.83% | 362 | +1.69% | 0 |  | 2,719 | -2.93% |
| Load/config_file | 182 | -0.55% | 164 | -1.20% | 0 |  | 2,849 | -0.80% |
| Render/config_file | 6,265 | +0.27% | 1,675 | +1.52% | 0 |  | 41,857 | -0.75% |
| Load/dict_ops | 38 | +15.15% | 23 | 0% | 0 |  | 1,235 | -2.29% |
| Render/dict_ops | 658 | +1.54% | 417 | +0.24% | 0 |  | 828 | -3.04% |
| Load/expressions | 36 | -14.29% | 34 | -8.11% | 0 |  | 1,211 | -2.10% |
| Render/expressions | 11 | -8.33% | 5 | 0% | 0 |  | 273 | -4.55% |
| Load/filters | 136 | +3.82% | 77 | 0% | 0 |  | 1,887 | -4.16% |
| Render/filters | 29 | -21.62% | 3 | -50.00% | 0 |  | 799 | -0.25% |
| Load/for_filter_if | 20 | +81.82% | 11 | +22.22% | 0 |  | 1,040 | -0.48% |
| Render/for_filter_if | 1,720 | -0.35% | 66 | +4.76% | 0 |  | 146 | -19.78% |
| Load/for_loop_vars | 46 | +84.00% | 26 | +30.00% | 0 |  | 1,040 | +0.97% |
| Render/for_loop_vars | 9 | +80.00% | 3 | +50.00% | 0 |  | 83 | -26.55% |
| Load/for_range | 13 | +85.71% | 12 | +50.00% | 0 |  | 704 | +3.38% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 116 | +63.38% |
| Load/html_autoescape | 418 | -3.69% | 502 | 0% | 0 |  | 3,863 | -3.79% |
| Render/html_autoescape | 4,848 | +25.69% | 1,480 | +8.90% | 0 |  | 37,031 | -1.24% |
| Load/inheritance | 9 | +80.00% | 7 | +40.00% | 0 |  | 597 | -1.32% |
| Render/inheritance | 588 | +0.17% | 125 | +3.31% | 0 |  | 741 | -1.59% |
| Load/large_static | 1,952 | +0.15% | 885 | 0% | 0 |  | 270 | -10.89% |
| Render/large_static | 707 | 0% | 571 | 0% | 0 |  | 3 | -25.00% |
| Load/macros | 74 | -8.64% | 52 | +4.00% | 0 |  | 1,591 | -0.56% |
| Render/macros | 760 | -0.65% | 240 | +0.84% | 0 |  | 1,041 | -22.54% |
| Load/many_tags | 27,712 | +3.49% | 20,660 | +0.28% | 0 |  | 230,906 | -2.21% |
| Render/many_tags | 6,057 | +0.20% | 867 | +1.76% | 0 |  | 135 | +8.00% |
| Load/mitsuhiko_table | 27 | +8.00% | 25 | +13.64% | 0 |  | 766 | -2.54% |
| Render/mitsuhiko_table | 9,281 | +0.13% | 6,609 | 0% | 0 |  | 297 | +2.06% |
| Load/mitsuhiko_table_wide | 84 | +10.53% | 116 | +2.65% | 0 |  | 1,250 | -3.10% |
| Render/mitsuhiko_table_wide | 9,398 | +0.23% | 22,906 | -0.01% | 0 |  | 287 | -5.90% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 5 | -64.29% |
| Render/plain_text | 0 |  | 1 |  | 0 |  | 3 | -40.00% |
| Load/strings | 158 | +5.33% | 99 | -8.33% | 0 |  | 2,670 | -2.98% |
| Render/strings | 492 | -1.20% | 603 | -0.17% | 0 |  | 1,424 | -0.77% |
| Load/substitute | 3 | +50.00% | 4 | +33.33% | 0 |  | 122 | -10.29% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 3 | -25.00% |

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
