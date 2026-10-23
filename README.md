# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (121 so far, latest [31091e8](https://github.com/jinja2cpp/Jinja2Cpp/commit/31091e805b6275a683429f9ee05c9246f900be1f) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,366 | 0% |  | 55 | 0% |  | 18,272 | 0% | -53.24% |
| [Render/chat_llama](#renderchat_llama) | 305,909 | -0.02% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 394,719 | +0.36% |  | 50 | 0% |  | 20,856 | +0.15% | -53.29% |
| [Render/chat_mistral](#renderchat_mistral) | 512,507 | +0.13% |  | 428 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,123 | -0.21% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 293,348 | +0.03% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,377 | -0.30% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,255,031 | +0.01% |  | 908 | 0% |  | 23,304 | 0% | -2.12% |
| [Load/dict_ops](#loaddict_ops) | 33,125 | -0.02% | -77.58% | 9 | 0% | -93.13% | 2,208 | 0% | -48.12% |
| [Render/dict_ops](#renderdict_ops) | 296,913 | -0.22% | -57.02% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,443 | +0.04% | -77.96% | 16 | 0% | -89.04% | 2,760 | 0% | -55.02% |
| [Render/expressions](#renderexpressions) | 426,152 | +0.09% | -52.97% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,818 | -0.05% | -73.83% | 27 | 0% | -88.16% | 3,768 | 0% | -66.99% |
| [Render/filters](#renderfilters) | 30,972 | -0.12% | -65.07% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,961 | +0.10% | -77.87% | 7 | 0% | -94.31% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,635 | +0.08% | -68.61% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,446 | +0.21% | -77.61% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,687 | -0.01% | -81.85% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,631 | +0.05% | -75.95% | 7 | 0% | -87.50% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 38,847 | +0.04% | -57.87% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,266 | -0.06% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,228,766 | -0.20% |  | 734 | 0% |  | 43,664 | 0% | +8.12% |
| [Load/inheritance](#loadinheritance) | 23,125 | -0.23% | -79.74% | 7 | 0% | -93.46% | 1,472 | 0% | -48.75% |
| [Render/inheritance](#renderinheritance) | 136,568 | +0.21% | -81.74% | 35 | 0% | -94.37% | 6,456 | 0% | +7.31% |
| [Load/int_arith](#loadint_arith) | 66,059 |  |  | 20 |  |  | 3,856 |  | 0% |
| [Render/int_arith](#renderint_arith) | 822,701 |  |  | 12 |  |  | 3,288 |  | 0% |
| [Load/large_static](#loadlarge_static) | 154,688 | -0.07% | -82.54% | 14 | 0% | -96.82% | 43,168 | 0% | -19.14% |
| [Render/large_static](#renderlarge_static) | 21,058 | 0% | -71.97% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,260 | -0.09% | -75.76% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 647,605 | -0.02% | -74.73% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,070,001 | -0.01% | -79.46% | 951 | 0% | -97.22% | 428,800 | +0.01% | -59.36% |
| [Render/many_tags](#rendermany_tags) | 724,923 | +0.04% | -65.20% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,195 | +0.03% | -80.55% | 19 | 0% | -89.67% | 3,408 | 0% | -43.80% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,183,549 | 0% | -58.05% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,635 | -0.13% |  | 35 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,528,906 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,782 | +0.19% | -60.91% | 4 | 0% | -77.78% | 720 | 0% | -35.71% |
| [Render/plain_text](#renderplain_text) | 1,032 | +0.10% | -84.81% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,185 | +0.03% | -75.27% | 29 | 0% | -87.05% | 4,296 | 0% | -52.48% |
| [Render/strings](#renderstrings) | 616,010 | +0.04% | -70.63% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,801 | +0.09% | -73.87% | 6 | 0% | -83.78% | 880 | 0% | -42.11% |
| [Render/substitute](#rendersubstitute) | 2,270 | +0.04% | -74.24% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 702 | -0.28% | 715 | -0.56% | 0 |  | 7,210 | -3.45% |
| Render/chat_llama | 1,110 | -4.06% | 473 | -6.52% | 0 |  | 3,595 | +4.63% |
| Load/chat_mistral | 1,087 | -0.73% | 1,025 | +0.10% | 0 |  | 8,165 | -1.11% |
| Render/chat_mistral | 1,824 | -1.51% | 538 | -2.00% | 0 |  | 8,819 | +0.35% |
| Load/chat_qwen | 495 | 0% | 585 | -3.94% | 0 |  | 4,398 | -3.74% |
| Render/chat_qwen | 998 | -8.94% | 386 | -8.31% | 0 |  | 3,081 | +4.23% |
| Load/config_file | 181 | +16.03% | 157 | -1.88% | 0 |  | 2,888 | -2.76% |
| Render/config_file | 6,382 | -1.16% | 1,916 | +1.81% | 0 |  | 37,769 | -3.70% |
| Load/dict_ops | 32 | +3.23% | 22 | +15.79% | 0 |  | 1,259 | -2.40% |
| Render/dict_ops | 749 | -2.98% | 440 | +6.80% | 0 |  | 840 | +1.69% |
| Load/expressions | 33 | +43.48% | 11 | -45.00% | 0 |  | 1,326 | +0.23% |
| Render/expressions | 6 | -14.29% | 3 | -25.00% | 0 |  | 267 | -16.30% |
| Load/filters | 135 | -2.88% | 69 | -4.17% | 0 |  | 2,241 | +6.36% |
| Render/filters | 29 | +123.08% | 9 | +80.00% | 0 |  | 761 | +1.20% |
| Load/for_filter_if | 6 | -45.45% | 2 | -60.00% | 0 |  | 1,039 | -1.33% |
| Render/for_filter_if | 1,648 | -0.54% | 63 | -8.70% | 0 |  | 148 | -9.20% |
| Load/for_loop_vars | 15 | -16.67% | 5 | -54.55% | 0 |  | 1,051 | -4.63% |
| Render/for_loop_vars | 4 | -50.00% | 2 | -50.00% | 0 |  | 131 | -6.43% |
| Load/for_range | 3 | +50.00% | 3 | 0% | 0 |  | 682 | -4.48% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 89 | -39.04% |
| Load/html_autoescape | 468 | +4.23% | 508 | -0.59% | 0 |  | 3,901 | -7.16% |
| Render/html_autoescape | 4,306 | -18.77% | 1,360 | -15.11% | 0 |  | 31,031 | -6.39% |
| Load/inheritance | 2 | -33.33% | 2 | -33.33% | 0 |  | 608 | -0.82% |
| Render/inheritance | 584 | +1.57% | 129 | -12.24% | 0 |  | 822 | -0.48% |
| Load/int_arith | 71 |  | 45 |  | 0 |  | 1,966 |  |
| Render/int_arith | 15 |  | 5 |  | 0 |  | 665 |  |
| Load/large_static | 1,942 | -0.41% | 907 | 0% | 0 |  | 282 | -14.02% |
| Render/large_static | 703 | 0% | 572 | +0.18% | 0 |  | 9 | +350.00% |
| Load/macros | 54 | -36.47% | 41 | -21.15% | 0 |  | 1,590 | -3.05% |
| Render/macros | 737 | +3.22% | 253 | +10.48% | 0 |  | 1,929 | +249.46% |
| Load/many_tags | 28,137 | -2.02% | 22,105 | -0.41% | 0 |  | 248,249 | -0.51% |
| Render/many_tags | 6,166 | +0.57% | 906 | +2.84% | 0 |  | 125 | -22.84% |
| Load/mitsuhiko_table | 9 | -71.88% | 9 | -79.55% | 0 |  | 796 | -3.28% |
| Render/mitsuhiko_table | 9,032 | -0.24% | 6,661 | +0.74% | 0 |  | 332 | +10.67% |
| Load/mitsuhiko_table_wide | 77 | -9.41% | 76 | -5.00% | 0 |  | 963 | -11.81% |
| Render/mitsuhiko_table_wide | 9,139 | +0.20% | 22,982 | +0.33% | 0 |  | 315 | +3.62% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | -62.16% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 5 | +25.00% |
| Load/strings | 184 | -2.13% | 113 | +13.00% | 0 |  | 2,815 | -1.47% |
| Render/strings | 481 | +3.66% | 604 | -0.49% | 0 |  | 1,456 | -0.14% |
| Load/substitute | 2 | +100.00% | 3 | 0% | 0 |  | 177 | -20.63% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 3 | +50.00% |

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

### Load/int_arith

![Load/int_arith](charts/Load_int_arith.svg)

### Render/int_arith

![Render/int_arith](charts/Render_int_arith.svg)

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
