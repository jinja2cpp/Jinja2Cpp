# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (106 so far, latest [21f311d](https://github.com/jinja2cpp/Jinja2Cpp/commit/21f311daaa608a28bcd730ea37384f2041187c8c) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 361,612 | 0% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 314,789 | 0% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 416,963 | 0% |  | 50 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 526,705 | +0.01% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 278,486 | 0% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 301,188 | 0% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,616 | 0% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,285,177 | +0.26% |  | 1,046 | 0% |  | 23,800 | +0.07% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 34,012 | -0.01% | -76.98% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 304,790 | 0% | -55.88% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,846 | 0% | -77.76% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 450,383 | 0% | -50.30% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,722 | -0.01% | -73.14% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,741 | +0.01% | -64.20% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,719 | 0% | -77.31% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,234 | 0% | -68.55% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,325 | 0% | -77.13% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 196,930 | 0% | -81.64% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,897 | -0.01% | -75.48% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,905 | +0.01% | -57.81% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,299 | 0% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,230,123 | +0.10% |  | 734 | 0% |  | 43,776 | +0.04% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,587 | -0.01% | -79.33% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 134,231 | +0.42% | -82.05% | 35 | +2.94% | -94.37% | 6,680 | +1.83% | +11.04% |
| [Load/large_static](#loadlarge_static) | 157,571 | 0% | -82.22% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,041 | +0.02% | -72.00% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,743 | 0% | -75.11% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 649,686 | +0.53% | -74.65% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,301,795 | 0% | -78.94% | 956 | 0% | -97.21% | 421,424 | 0% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,460 | 0% | -64.74% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,955 | -0.02% | -79.81% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,147,602 | 0% | -58.34% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,675 | 0% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,492,842 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,721 | +0.03% | -61.55% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,020 | +0.39% | -84.99% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,983 | -0.01% | -74.65% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 611,777 | 0% | -70.83% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,864 | +0.01% | -73.66% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,248 | +0.18% | -74.50% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 746 | -0.67% | 726 | -0.41% | 0 |  | 7,979 | +1.37% |
| Render/chat_llama | 1,054 | +0.67% | 420 | 0% | 0 |  | 3,455 | +2.37% |
| Load/chat_mistral | 1,082 | -1.10% | 1,005 | +0.10% | 0 |  | 8,935 | +2.81% |
| Render/chat_mistral | 1,736 | -2.25% | 418 | 0% | 0 |  | 8,817 | -5.20% |
| Load/chat_qwen | 526 | +0.38% | 612 | +0.66% | 0 |  | 4,962 | +11.98% |
| Render/chat_qwen | 958 | -0.10% | 317 | -2.16% | 0 |  | 2,792 | -2.48% |
| Load/config_file | 183 | -6.15% | 183 | -3.17% | 0 |  | 3,020 | +1.04% |
| Render/config_file | 5,910 | +2.28% | 1,403 | +2.63% | 0 |  | 38,255 | -4.94% |
| Load/dict_ops | 34 | -5.56% | 22 | +15.79% | 0 |  | 1,273 | -0.31% |
| Render/dict_ops | 695 | -0.71% | 433 | -0.69% | 0 |  | 822 | -6.48% |
| Load/expressions | 35 | -16.67% | 26 | -10.34% | 0 |  | 1,230 | -4.58% |
| Render/expressions | 6 | +20.00% | 4 | +33.33% | 0 |  | 269 | -16.72% |
| Load/filters | 112 | -5.88% | 62 | +6.90% | 0 |  | 1,902 | -4.33% |
| Render/filters | 17 | +6.25% | 5 | 0% | 0 |  | 792 | -4.12% |
| Load/for_filter_if | 17 | +21.43% | 7 | 0% | 0 |  | 1,092 | +1.30% |
| Render/for_filter_if | 1,725 | +0.58% | 70 | -2.78% | 0 |  | 180 | +0.56% |
| Load/for_loop_vars | 32 | -13.51% | 23 | -4.17% | 0 |  | 1,103 | +2.89% |
| Render/for_loop_vars | 7 | +40.00% | 6 | +50.00% | 0 |  | 123 | -0.81% |
| Load/for_range | 3 | 0% | 5 | 0% | 0 |  | 692 | -5.98% |
| Render/for_range | 2 | -33.33% | 0 | -100.00% | 0 |  | 75 | -36.97% |
| Load/html_autoescape | 484 | +1.04% | 509 | +0.59% | 0 |  | 4,287 | +5.07% |
| Render/html_autoescape | 3,971 | +17.07% | 1,062 | +1.24% | 0 |  | 33,592 | +7.76% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 627 | +0.48% |
| Render/inheritance | 579 | +2.48% | 133 | +0.76% | 0 |  | 841 | +5.65% |
| Load/large_static | 1,949 | +0.05% | 895 | 0% | 0 |  | 315 | +25.00% |
| Render/large_static | 704 | +0.14% | 574 | 0% | 0 |  | 2 | -81.82% |
| Load/macros | 71 | +4.41% | 58 | +1.75% | 0 |  | 1,671 | +0.54% |
| Render/macros | 714 | +0.42% | 242 | -0.41% | 0 |  | 556 | +8.81% |
| Load/many_tags | 28,440 | -1.81% | 21,609 | -0.48% | 0 |  | 248,763 | -5.00% |
| Render/many_tags | 6,043 | +0.07% | 872 | 0% | 0 |  | 145 | +16.94% |
| Load/mitsuhiko_table | 30 | +3.45% | 39 | +5.41% | 0 |  | 845 | +0.84% |
| Render/mitsuhiko_table | 9,035 | +0.14% | 6,618 | +0.05% | 0 |  | 285 | -5.63% |
| Load/mitsuhiko_table_wide | 80 | +1.27% | 74 | -7.50% | 0 |  | 957 | -9.29% |
| Render/mitsuhiko_table_wide | 9,262 | +0.79% | 22,939 | +0.07% | 0 |  | 295 | -3.59% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 15 | +200.00% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 4 | -33.33% |
| Load/strings | 160 | -1.23% | 106 | +0.95% | 0 |  | 2,810 | -0.92% |
| Render/strings | 482 | +1.69% | 619 | +0.16% | 0 |  | 1,461 | +1.67% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 158 | +53.40% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 2 | -50.00% |

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
