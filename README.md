# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (114 so far, latest [3985100](https://github.com/jinja2cpp/Jinja2Cpp/commit/39851009a464469d0331890071cdba3f061972f8) on 2026-10-19). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,672 | +0.05% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 310,704 | -1.33% |  | 262 | 0% |  | 10,232 | 0% | -28.75% |
| [Load/chat_mistral](#loadchat_mistral) | 399,869 | -0.11% |  | 50 | 0% |  | 20,680 | -0.08% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 519,263 | -1.36% |  | 448 | 0% |  | 9,072 | 0% | -31.31% |
| [Load/chat_qwen](#loadchat_qwen) | 269,004 | +0.07% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 295,539 | -1.86% |  | 249 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,289 | -0.31% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,285,837 | -0.41% |  | 1,046 | 0% |  | 23,800 | 0% | -0.03% |
| [Load/dict_ops](#loaddict_ops) | 33,680 | -0.10% | -77.21% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 307,808 | +0.08% | -55.44% | 112 | 0% | -75.11% | 24,408 | -0.07% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,603 | -0.01% | -77.88% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 454,433 | +0.74% | -49.85% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 73,317 | 0% | -73.28% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 30,970 | -2.45% | -65.07% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,462 | +0.17% | -77.50% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,484 | -0.44% | -68.63% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,899 | -0.01% | -77.36% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,825 | -0.83% | -81.84% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,811 | +0.15% | -75.63% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,368 | +0.17% | -57.31% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 198,285 | -0.17% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,234,186 | -0.68% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,447 | +0.63% | -79.45% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,663 | +0.13% | -81.73% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 156,177 | 0% | -82.37% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,072 | +0.03% | -71.96% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,176 | 0% | -75.36% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 650,015 | +0.09% | -74.64% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,207,706 | -0.04% | -79.15% | 956 | 0% | -97.21% | 421,328 | -0.02% | -60.07% |
| [Render/many_tags](#rendermany_tags) | 732,889 | -0.22% | -64.82% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,518 | -0.08% | -80.00% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,197,671 | +0.23% | -57.93% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,033 | +1.03% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,880 | +0.22% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,741 | +0.56% | -61.34% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,051 | +0.69% | -84.53% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,399 | -0.12% | -74.85% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 621,296 | -0.51% | -70.38% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,828 | +0.27% | -73.78% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,279 | +0.32% | -74.14% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 738 | +3.51% | 732 | +1.24% | 0 |  | 7,283 | +1.01% |
| Render/chat_llama | 1,045 | -2.43% | 409 | -3.54% | 0 |  | 3,283 | -2.26% |
| Load/chat_mistral | 1,042 | -2.07% | 1,031 | +0.88% | 0 |  | 8,214 | +1.94% |
| Render/chat_mistral | 1,686 | -5.76% | 433 | +3.84% | 0 |  | 8,926 | -0.99% |
| Load/chat_qwen | 506 | +2.22% | 602 | +1.01% | 0 |  | 4,392 | +4.67% |
| Render/chat_qwen | 957 | -6.36% | 324 | +5.19% | 0 |  | 2,811 | -2.09% |
| Load/config_file | 173 | +0.58% | 180 | +1.12% | 0 |  | 2,895 | +1.15% |
| Render/config_file | 6,103 | -3.75% | 1,777 | -2.74% | 0 |  | 38,741 | -2.04% |
| Load/dict_ops | 26 | -25.71% | 19 | +5.56% | 0 |  | 1,296 | +2.69% |
| Render/dict_ops | 676 | -1.74% | 426 | -0.47% | 0 |  | 831 | -3.37% |
| Load/expressions | 35 | -7.89% | 25 | +13.64% | 0 |  | 1,304 | +1.72% |
| Render/expressions | 2 | -33.33% | 1 | 0% | 0 |  | 279 | -22.50% |
| Load/filters | 127 | +14.41% | 61 | 0% | 0 |  | 2,003 | +2.98% |
| Render/filters | 26 | -10.34% | 8 | 0% | 0 |  | 746 | -7.56% |
| Load/for_filter_if | 5 | -66.67% | 4 | -55.56% | 0 |  | 1,120 | +4.97% |
| Render/for_filter_if | 1,653 | -3.39% | 65 | 0% | 0 |  | 160 | -4.19% |
| Load/for_loop_vars | 21 | -38.24% | 12 | -50.00% | 0 |  | 1,105 | +6.97% |
| Render/for_loop_vars | 14 | +100.00% | 8 | +60.00% | 0 |  | 99 | -38.12% |
| Load/for_range | 6 | +100.00% | 5 | 0% | 0 |  | 710 | +1.14% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 112 | -5.88% |
| Load/html_autoescape | 416 | -12.79% | 502 | -0.59% | 0 |  | 4,263 | +5.86% |
| Render/html_autoescape | 4,150 | +5.01% | 1,434 | +13.72% | 0 |  | 32,460 | +0.23% |
| Load/inheritance | 9 | +200.00% | 7 | +75.00% | 0 |  | 584 | -5.19% |
| Render/inheritance | 576 | +1.41% | 114 | -8.80% | 0 |  | 800 | -6.43% |
| Load/large_static | 1,944 | -0.10% | 892 | -0.11% | 0 |  | 263 | -10.54% |
| Render/large_static | 707 | +0.43% | 572 | -0.52% | 0 |  | 11 | +175.00% |
| Load/macros | 71 | -1.39% | 61 | +5.17% | 0 |  | 1,662 | +3.29% |
| Render/macros | 781 | +7.58% | 250 | +0.81% | 0 |  | 510 | -2.67% |
| Load/many_tags | 27,757 | +0.85% | 21,163 | -1.27% | 0 |  | 250,358 | +2.88% |
| Render/many_tags | 6,015 | -0.46% | 844 | -3.43% | 0 |  | 176 | +58.56% |
| Load/mitsuhiko_table | 27 | +3.85% | 39 | +5.41% | 0 |  | 810 | -0.49% |
| Render/mitsuhiko_table | 9,066 | +0.44% | 6,497 | -1.77% | 0 |  | 304 | -16.48% |
| Load/mitsuhiko_table_wide | 69 | -22.47% | 68 | -13.92% | 0 |  | 1,021 | +7.59% |
| Render/mitsuhiko_table_wide | 9,135 | -0.22% | 22,961 | +0.17% | 0 |  | 292 | -14.37% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 43 | +760.00% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 4 | -42.86% |
| Load/strings | 166 | +7.10% | 99 | -11.61% | 0 |  | 2,751 | +1.36% |
| Render/strings | 488 | +2.31% | 611 | -0.97% | 0 |  | 1,420 | -2.14% |
| Load/substitute | 2 | -33.33% | 4 | 0% | 0 |  | 186 | +2.20% |
| Render/substitute | 0 |  | 0 | -100.00% | 0 |  | 2 | -33.33% |

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
