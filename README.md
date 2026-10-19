# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (118 so far, latest [5fe00b9](https://github.com/jinja2cpp/Jinja2Cpp/commit/5fe00b92b65e5ae944e68ea33cd17bd5ca22c3a8) on 2026-10-19). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 353,762 | -0.97% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 306,552 | -0.49% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 405,478 | -0.83% |  | 50 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 514,767 | -0.05% |  | 428 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 272,469 | -0.81% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 293,798 | +0.14% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 101,700 | -0.80% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,259,410 | +0.13% |  | 908 | 0% |  | 23,752 | 0% | -0.24% |
| [Load/dict_ops](#loaddict_ops) | 34,217 | -0.72% | -76.85% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 299,473 | +0.23% | -56.65% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 46,405 | -0.45% | -77.49% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 454,321 | 0% | -49.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 74,152 | -0.49% | -72.98% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,165 | -0.28% | -64.85% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,929 | -0.83% | -77.16% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,312 | -0.01% | -68.65% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,498 | -0.99% | -77.03% | 10 | 0% | -93.63% | 2,352 | 0% | -47.96% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,646 | +0.01% | -81.86% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 14,048 | -0.66% | -75.22% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,258 | +0.05% | -57.43% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 201,632 | -0.47% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,234,109 | +0.10% |  | 734 | 0% |  | 43,776 | 0% | +8.40% |
| [Load/inheritance](#loadinheritance) | 23,816 | -1.06% | -79.13% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,787 | +0.08% | -81.71% | 35 | 0% | -94.37% | 6,680 | 0% | +11.04% |
| [Load/large_static](#loadlarge_static) | 157,140 | -1.32% | -82.26% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,085 | +0.04% | -71.94% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 57,050 | -0.63% | -74.97% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 648,680 | +0.01% | -74.69% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,325,902 | -1.11% | -78.88% | 956 | 0% | -97.21% | 421,440 | +0.03% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 734,811 | +0.02% | -64.72% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 48,188 | -0.85% | -79.72% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,197,255 | -0.01% | -57.94% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 57,103 | -0.83% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,970 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,844 | +0.79% | -60.27% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,059 | +0.76% | -84.41% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 74,478 | -0.46% | -74.48% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 633,241 | -0.04% | -69.81% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,961 | -0.72% | -73.34% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,297 | +0.35% | -73.94% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 697 | -4.26% | 727 | -0.14% | 0 |  | 7,735 | +4.65% |
| Render/chat_llama | 1,168 | +5.23% | 487 | +0.41% | 0 |  | 3,453 | +4.57% |
| Load/chat_mistral | 1,000 | -4.12% | 994 | -4.33% | 0 |  | 8,643 | +8.34% |
| Render/chat_mistral | 1,841 | +2.45% | 558 | +9.63% | 0 |  | 8,963 | +1.98% |
| Load/chat_qwen | 520 | +3.59% | 612 | +1.83% | 0 |  | 4,713 | +13.10% |
| Render/chat_qwen | 1,065 | -2.20% | 408 | -2.16% | 0 |  | 3,078 | +0.75% |
| Load/config_file | 172 | +13.16% | 179 | +1.13% | 0 |  | 2,966 | +5.78% |
| Render/config_file | 5,904 | +0.31% | 1,837 | +8.12% | 0 |  | 38,432 | -3.16% |
| Load/dict_ops | 40 | +90.48% | 27 | +125.00% | 0 |  | 1,358 | +7.35% |
| Render/dict_ops | 768 | +1.86% | 427 | -0.23% | 0 |  | 867 | +2.24% |
| Load/expressions | 36 | +89.47% | 27 | +145.45% | 0 |  | 1,340 | +6.60% |
| Render/expressions | 8 | +60.00% | 2 | -50.00% | 0 |  | 309 | +23.60% |
| Load/filters | 126 | -0.79% | 79 | +46.30% | 0 |  | 1,869 | -10.70% |
| Render/filters | 33 | +10.00% | 7 | -12.50% | 0 |  | 748 | -1.32% |
| Load/for_filter_if | 8 | -55.56% | 8 | 0% | 0 |  | 1,109 | +7.05% |
| Render/for_filter_if | 1,662 | +0.61% | 75 | +19.05% | 0 |  | 179 | -13.53% |
| Load/for_loop_vars | 18 | 0% | 14 | +16.67% | 0 |  | 1,100 | +7.42% |
| Render/for_loop_vars | 12 | 0% | 7 | -22.22% | 0 |  | 145 | +22.88% |
| Load/for_range | 2 | -33.33% | 5 | +25.00% | 0 |  | 736 | +4.99% |
| Render/for_range | 1 | 0% | 1 |  | 0 |  | 114 | +14.00% |
| Load/html_autoescape | 442 | +4.99% | 485 | -2.81% | 0 |  | 4,032 | -0.67% |
| Render/html_autoescape | 3,694 | -0.46% | 1,320 | +9.45% | 0 |  | 31,165 | +1.46% |
| Load/inheritance | 2 | -33.33% | 5 | +66.67% | 0 |  | 672 | +10.53% |
| Render/inheritance | 596 | +2.76% | 131 | +23.58% | 0 |  | 798 | +0.88% |
| Load/large_static | 1,955 | +0.36% | 895 | +0.56% | 0 |  | 304 | +22.09% |
| Render/large_static | 709 | +0.14% | 571 | -0.17% | 0 |  | 15 | +15.38% |
| Load/macros | 82 | +20.59% | 64 | +33.33% | 0 |  | 1,712 | +6.53% |
| Render/macros | 738 | +3.07% | 251 | +0.40% | 0 |  | 2,262 | -1.27% |
| Load/many_tags | 27,762 | -1.26% | 21,444 | +1.04% | 0 |  | 246,780 | -3.45% |
| Render/many_tags | 6,031 | +0.15% | 863 | +2.49% | 0 |  | 156 | +2.63% |
| Load/mitsuhiko_table | 28 | +7.69% | 44 | +4.76% | 0 |  | 907 | +13.23% |
| Render/mitsuhiko_table | 9,063 | +0.03% | 6,618 | +1.83% | 0 |  | 3,159 | +881.06% |
| Load/mitsuhiko_table_wide | 70 | +4.48% | 80 | +21.21% | 0 |  | 968 | +7.68% |
| Render/mitsuhiko_table_wide | 9,142 | -0.22% | 22,911 | -0.21% | 0 |  | 299 | -10.21% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 8 | +14.29% |
| Load/strings | 162 | +2.53% | 109 | +14.74% | 0 |  | 2,797 | +4.37% |
| Render/strings | 468 | -7.33% | 613 | +1.16% | 0 |  | 1,484 | -0.27% |
| Load/substitute | 2 | +100.00% | 4 | +33.33% | 0 |  | 157 | +17.16% |
| Render/substitute | 0 |  | 1 |  | 0 |  | 13 | +550.00% |

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
