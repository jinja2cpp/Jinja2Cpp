# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (90 so far, latest [2b63d2d](https://github.com/jinja2cpp/Jinja2Cpp/commit/2b63d2d3c3f66aa20bfc9c7291f46d9df81aa730) on 2026-10-16). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 355,812 | +0.03% |  | 59 | 0% |  | 18,064 | 0% | -53.78% |
| [Render/chat_llama](#renderchat_llama) | 307,409 | +0.35% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 411,389 | +0.01% |  | 52 | 0% |  | 20,664 | 0% | -53.72% |
| [Render/chat_mistral](#renderchat_mistral) | 516,169 | +0.37% |  | 450 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 276,043 | +0.02% |  | 45 | 0% |  | 14,304 | 0% | -47.66% |
| [Render/chat_qwen](#renderchat_qwen) | 292,972 | +0.31% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,578 | +0.06% |  | 27 | 0% |  | 5,320 | 0% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,331,038 | -0.52% |  | 1,069 | -0.28% |  | 23,304 | -1.52% | -2.12% |
| [Load/dict_ops](#loaddict_ops) | 33,632 | -0.01% | -77.24% | 9 | 0% | -93.13% | 2,144 | 0% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 301,778 | -0.23% | -56.32% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,247 | -0.01% | -78.05% | 16 | 0% | -89.04% | 2,712 | 0% | -55.80% |
| [Render/expressions](#renderexpressions) | 447,723 | 0% | -50.59% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,881 | +0.26% | -73.44% | 29 | 0% | -87.28% | 3,656 | 0% | -67.97% |
| [Render/filters](#renderfilters) | 30,958 | -2.77% | -65.09% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,373 | -0.04% | -77.57% | 7 | 0% | -94.31% | 1,888 | 0% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 270,529 | 0% | -68.74% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,886 | +0.09% | -77.37% | 10 | 0% | -93.63% | 2,336 | 0% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,246 | 0% | -81.89% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,744 | -0.04% | -75.75% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,642 | +0.02% | -58.09% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,907 | +0.05% |  | 50 | 0% |  | 12,304 | 0% | -50.15% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,259,935 | +0.48% |  | 747 | -0.27% |  | 42,864 | -0.63% | +6.14% |
| [Load/inheritance](#loadinheritance) | 23,275 | -0.03% | -79.60% | 7 | 0% | -93.46% | 1,408 | 0% | -50.97% |
| [Render/inheritance](#renderinheritance) | 135,048 | +0.05% | -81.94% | 34 | 0% | -94.53% | 6,512 | 0% | +8.24% |
| [Load/large_static](#loadlarge_static) | 156,382 | 0% | -82.35% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,100 | 0% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,047 | 0% | -75.41% | 28 | 0% | -84.95% | 4,296 | 0% | -30.89% |
| [Render/macros](#rendermacros) | 644,241 | +0.31% | -74.86% | 308 | -0.32% | -88.71% | 18,848 | -0.97% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,193,131 | +0.05% | -79.18% | 945 | 0% | -97.24% | 421,312 | 0% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 728,830 | 0% | -65.01% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,453 | -0.01% | -80.03% | 20 | 0% | -89.13% | 3,312 | 0% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,117,050 | 0% | -58.59% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 66,496 | -0.01% |  | 45 | 0% |  | 5,640 | 0% | -31.75% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,462,413 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,689 | -0.16% | -61.88% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,001 | 0% | -85.27% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,088 | +0.14% | -74.96% | 30 | 0% | -86.61% | 4,216 | 0% | -53.36% |
| [Render/strings](#renderstrings) | 600,065 | -0.03% | -71.39% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,784 | -0.08% | -73.93% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,228 | 0% | -74.72% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 683 | +1.79% | 693 | +1.02% | 0 |  | 6,974 | -2.94% |
| Render/chat_llama | 1,143 | +2.05% | 441 | +2.80% | 0 |  | 3,531 | +2.71% |
| Load/chat_mistral | 1,007 | +0.80% | 973 | -0.71% | 0 |  | 7,963 | -1.56% |
| Render/chat_mistral | 1,796 | +1.18% | 473 | +3.50% | 0 |  | 9,770 | +7.98% |
| Load/chat_qwen | 524 | +4.38% | 600 | +0.17% | 0 |  | 4,266 | -5.26% |
| Render/chat_qwen | 1,053 | +2.33% | 370 | +2.21% | 0 |  | 2,879 | -1.61% |
| Load/config_file | 189 | +3.85% | 168 | +2.44% | 0 |  | 2,730 | -5.24% |
| Render/config_file | 6,203 | -0.99% | 1,630 | -2.69% | 0 |  | 42,032 | +4.79% |
| Load/dict_ops | 29 | -23.68% | 20 | -13.04% | 0 |  | 1,220 | -3.56% |
| Render/dict_ops | 643 | -2.28% | 422 | +1.20% | 0 |  | 842 | -2.43% |
| Load/expressions | 33 | -8.33% | 29 | -14.71% | 0 |  | 1,228 | -1.76% |
| Render/expressions | 11 | 0% | 5 | 0% | 0 |  | 268 | +0.75% |
| Load/filters | 141 | +3.68% | 74 | -3.90% | 0 |  | 1,859 | -4.72% |
| Render/filters | 18 | -37.93% | 5 | +66.67% | 0 |  | 789 | -2.59% |
| Load/for_filter_if | 14 | -30.00% | 11 | 0% | 0 |  | 1,015 | -1.65% |
| Render/for_filter_if | 1,721 | +0.06% | 66 | 0% | 0 |  | 173 | -2.26% |
| Load/for_loop_vars | 32 | -30.43% | 19 | -26.92% | 0 |  | 1,020 | -3.13% |
| Render/for_loop_vars | 7 | -22.22% | 3 | 0% | 0 |  | 131 | -1.50% |
| Load/for_range | 8 | -38.46% | 10 | -16.67% | 0 |  | 706 | -3.02% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 119 | +0.85% |
| Load/html_autoescape | 448 | +7.18% | 495 | -1.39% | 0 |  | 3,832 | -3.23% |
| Render/html_autoescape | 5,139 | +6.00% | 1,776 | +20.00% | 0 |  | 36,227 | -1.02% |
| Load/inheritance | 3 | -66.67% | 2 | -71.43% | 0 |  | 591 | -4.21% |
| Render/inheritance | 592 | +0.68% | 129 | +3.20% | 0 |  | 768 | +6.37% |
| Load/large_static | 1,956 | +0.20% | 886 | +0.11% | 0 |  | 268 | -19.03% |
| Render/large_static | 709 | +0.28% | 571 | 0% | 0 |  | 2 | -50.00% |
| Load/macros | 74 | 0% | 54 | +3.85% | 0 |  | 1,565 | -2.43% |
| Render/macros | 753 | -0.92% | 240 | 0% | 0 |  | 829 | -28.96% |
| Load/many_tags | 26,828 | -3.19% | 20,614 | -0.22% | 0 |  | 237,948 | +0.80% |
| Render/many_tags | 6,049 | -0.13% | 874 | +0.81% | 0 |  | 194 | +46.97% |
| Load/mitsuhiko_table | 20 | -25.93% | 19 | -24.00% | 0 |  | 769 | -5.88% |
| Render/mitsuhiko_table | 9,292 | +0.12% | 6,611 | +0.03% | 0 |  | 304 | +6.67% |
| Load/mitsuhiko_table_wide | 95 | +13.10% | 112 | -3.45% | 0 |  | 1,162 | -8.79% |
| Render/mitsuhiko_table_wide | 9,388 | -0.11% | 22,917 | +0.05% | 0 |  | 285 | +0.71% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | 0% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 6 | +50.00% |
| Load/strings | 172 | +8.86% | 113 | +14.14% | 0 |  | 2,682 | -1.79% |
| Render/strings | 499 | +1.42% | 606 | +0.50% | 0 |  | 1,435 | -16.67% |
| Load/substitute | 2 | -33.33% | 3 | -25.00% | 0 |  | 124 | -13.29% |
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
