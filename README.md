# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (101 so far, latest [7f4b20b](https://github.com/jinja2cpp/Jinja2Cpp/commit/7f4b20b7d16a561bf7d9f889f8efb5148f4b72e1) on 2026-10-17). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 360,708 | +0.79% |  | 57 | 0% |  | 18,080 | +0.09% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 313,675 | -0.11% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 416,072 | +0.54% |  | 50 | 0% |  | 20,696 | +0.08% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 523,408 | +0.25% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 278,115 | +0.17% |  | 44 | 0% |  | 14,320 | +0.11% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 299,649 | -0.06% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,603 | +0.10% |  | 27 | 0% |  | 5,336 | +0.30% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,271,361 | +0.58% |  | 1,046 | 0% |  | 23,768 | +0.07% | -0.17% |
| [Load/dict_ops](#loaddict_ops) | 33,959 | +0.06% | -77.02% | 9 | 0% | -93.13% | 2,160 | +0.75% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 303,882 | +0.04% | -56.01% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,836 | +0.58% | -77.77% | 16 | 0% | -89.04% | 2,728 | +0.59% | -55.54% |
| [Render/expressions](#renderexpressions) | 449,925 | 0% | -50.35% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,262 | -0.05% | -73.30% | 29 | 0% | -87.28% | 3,672 | +0.44% | -67.83% |
| [Render/filters](#renderfilters) | 31,428 | +0.04% | -64.56% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,724 | +0.06% | -77.31% | 7 | 0% | -94.31% | 1,904 | +0.85% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 269,851 | +0.01% | -68.82% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,296 | +0.02% | -77.14% | 10 | 0% | -93.63% | 2,352 | +0.68% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 195,299 | +0.02% | -81.79% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,903 | +0.02% | -75.47% | 7 | 0% | -87.50% | 1,104 | +1.47% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,647 | +0.04% | -58.09% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,123 | +0.06% |  | 50 | 0% |  | 12,320 | +0.13% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,227,029 | +0.02% |  | 734 | 0% |  | 43,744 | +0.04% | +8.32% |
| [Load/inheritance](#loadinheritance) | 23,602 | +0.01% | -79.32% | 7 | 0% | -93.46% | 1,424 | +1.14% | -50.42% |
| [Render/inheritance](#renderinheritance) | 132,350 | +0.88% | -82.30% | 34 | 0% | -94.53% | 6,560 | 0% | +9.04% |
| [Load/large_static](#loadlarge_static) | 157,474 | 0% | -82.23% | 18 | 0% | -95.91% | 42,736 | +0.04% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,029 | +0.07% | -72.01% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,730 | +0.07% | -75.11% | 28 | 0% | -84.95% | 4,312 | +0.37% | -30.63% |
| [Render/macros](#rendermacros) | 639,642 | +0.13% | -75.04% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,268,459 | 0% | -79.01% | 956 | 0% | -97.21% | 421,424 | +0.01% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 729,646 | 0% | -64.97% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,899 | -0.01% | -79.84% | 20 | 0% | -89.13% | 3,328 | +0.48% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,105,488 | 0% | -58.68% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,611 | -0.76% |  | 36 | 0% |  | 5,656 | +0.28% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,450,728 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,732 | +0.08% | -61.43% | 4 | 0% | -77.78% | 704 | +2.33% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,014 | +1.40% | -85.08% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,704 | 0% | -74.75% | 30 | 0% | -86.61% | 4,232 | +0.38% | -53.19% |
| [Render/strings](#renderstrings) | 609,651 | +0.22% | -70.93% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,871 | +0.04% | -73.64% | 6 | 0% | -83.78% | 848 | +1.92% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,232 | +0.59% | -74.68% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 751 | +2.88% | 735 | +1.10% | 0 |  | 7,832 | +15.30% |
| Render/chat_llama | 1,050 | -2.60% | 416 | -3.26% | 0 |  | 3,376 | +7.58% |
| Load/chat_mistral | 1,083 | +1.21% | 1,009 | -0.10% | 0 |  | 8,827 | +17.05% |
| Render/chat_mistral | 1,784 | -0.56% | 419 | -1.18% | 0 |  | 8,496 | -7.32% |
| Load/chat_qwen | 528 | +2.72% | 613 | -1.29% | 0 |  | 4,787 | +15.71% |
| Render/chat_qwen | 929 | -1.06% | 309 | +0.65% | 0 |  | 2,926 | +4.09% |
| Load/config_file | 185 | +2.78% | 191 | +2.14% | 0 |  | 3,055 | +7.34% |
| Render/config_file | 5,948 | -2.36% | 1,373 | +4.57% | 0 |  | 37,416 | +2.14% |
| Load/dict_ops | 32 | +3.23% | 18 | +12.50% | 0 |  | 1,294 | +3.69% |
| Render/dict_ops | 700 | +0.43% | 431 | 0% | 0 |  | 851 | +6.91% |
| Load/expressions | 31 | +3.33% | 28 | +12.00% | 0 |  | 1,342 | +11.93% |
| Render/expressions | 6 | +20.00% | 4 | 0% | 0 |  | 300 | +17.19% |
| Load/filters | 126 | +12.50% | 67 | +19.64% | 0 |  | 1,906 | -7.83% |
| Render/filters | 19 | +5.56% | 4 | -33.33% | 0 |  | 789 | -2.11% |
| Load/for_filter_if | 11 | +37.50% | 7 | +16.67% | 0 |  | 1,106 | +5.13% |
| Render/for_filter_if | 1,710 | -0.35% | 68 | +1.49% | 0 |  | 159 | +11.19% |
| Load/for_loop_vars | 29 | -9.38% | 26 | +23.81% | 0 |  | 1,097 | +5.68% |
| Render/for_loop_vars | 6 | 0% | 5 | +66.67% | 0 |  | 128 | +9.40% |
| Load/for_range | 3 | 0% | 4 | -20.00% | 0 |  | 751 | +2.32% |
| Render/for_range | 3 | +200.00% | 3 | +200.00% | 0 |  | 151 | +93.59% |
| Load/html_autoescape | 489 | +1.24% | 502 | 0% | 0 |  | 4,297 | +7.61% |
| Render/html_autoescape | 3,187 | +11.94% | 1,048 | -5.24% | 0 |  | 31,618 | +2.27% |
| Load/inheritance | 2 | 0% | 3 | 0% | 0 |  | 631 | +0.80% |
| Render/inheritance | 570 | +0.18% | 132 | -1.49% | 0 |  | 809 | +7.29% |
| Load/large_static | 1,945 | -0.15% | 897 | 0% | 0 |  | 317 | +22.39% |
| Render/large_static | 701 | +0.14% | 576 | 0% | 0 |  | 3 | -57.14% |
| Load/macros | 69 | 0% | 61 | 0% | 0 |  | 1,688 | +3.75% |
| Render/macros | 709 | -0.84% | 244 | +1.24% | 0 |  | 520 | -50.33% |
| Load/many_tags | 27,784 | +0.64% | 21,522 | +0.27% | 0 |  | 254,445 | +6.07% |
| Render/many_tags | 6,036 | +0.07% | 876 | -0.57% | 0 |  | 79 | -17.71% |
| Load/mitsuhiko_table | 22 | +46.67% | 26 | +44.44% | 0 |  | 827 | +2.61% |
| Render/mitsuhiko_table | 9,025 | -0.17% | 6,614 | +0.11% | 0 |  | 317 | +8.93% |
| Load/mitsuhiko_table_wide | 80 | +15.94% | 86 | +16.22% | 0 |  | 998 | +3.74% |
| Render/mitsuhiko_table_wide | 9,177 | -0.12% | 22,928 | +0.03% | 0 |  | 327 | +9.73% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 6 | +100.00% |
| Load/strings | 172 | +7.50% | 114 | +11.76% | 0 |  | 2,779 | +0.76% |
| Render/strings | 472 | -0.84% | 616 | 0% | 0 |  | 1,433 | +1.06% |
| Load/substitute | 2 | 0% | 5 | 0% | 0 |  | 176 | +20.55% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |

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
