# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (56 so far, latest [d509a09](https://github.com/jinja2cpp/Jinja2Cpp/commit/d509a0912674a0e692685f0c47121b6641c7491d) on 2026-10-08). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 565,664 | -0.01% |  | 92 | 0% |  | 20,928 | 0% | -46.45% |
| [Render/chat_llama](#renderchat_llama) | 411,330 | +0.32% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 668,886 | +0.01% |  | 74 | 0% |  | 25,288 | 0% | -43.36% |
| [Render/chat_mistral](#renderchat_mistral) | 710,372 | +0.15% |  | 474 | 0% |  | 11,632 | 0% | -11.93% |
| [Load/chat_qwen](#loadchat_qwen) | 427,741 | +0.01% |  | 68 | 0% |  | 16,256 | 0% | -40.52% |
| [Render/chat_qwen](#renderchat_qwen) | 397,441 | +0.32% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 148,860 | +0.02% |  | 34 | 0% |  | 5,928 | 0% | -43.86% |
| [Render/config_file](#renderconfig_file) | 1,914,475 | +0.02% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 52,401 | 0% | -64.54% | 15 | 0% | -88.55% | 2,520 | 0% | -40.79% |
| [Render/dict_ops](#renderdict_ops) | 400,723 | 0% | -42.00% | 315 | 0% | -30.00% | 34,032 | 0% | -1.91% |
| [Load/expressions](#loadexpressions) | 68,207 | 0% | -66.91% | 18 | 0% | -87.67% | 3,656 | 0% | -40.42% |
| [Render/expressions](#renderexpressions) | 518,965 | 0% | -42.73% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,407 | 0% | -64.51% | 32 | 0% | -85.96% | 3,960 | 0% | -65.31% |
| [Render/filters](#renderfilters) | 60,297 | +0.11% | -32.00% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 41,870 | +0.03% | -69.07% | 9 | 0% | -92.68% | 2,032 | 0% | -41.07% |
| [Render/for_filter_if](#renderfor_filter_if) | 461,272 | 0% | -46.70% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 57,889 | +0.03% | -67.96% | 13 | 0% | -91.72% | 2,512 | 0% | -44.42% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 275,041 | 0% | -74.36% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 20,600 | 0% | -63.66% | 10 | 0% | -82.14% | 1,200 | 0% | -40.00% |
| [Render/for_range](#renderfor_range) | 46,735 | 0% | -49.32% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 290,910 | +0.02% |  | 63 | 0% |  | 13,624 | 0% | -44.80% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,612,040 | -0.03% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 35,986 | 0% | -68.47% | 10 | 0% | -90.65% | 1,616 | 0% | -43.73% |
| [Render/inheritance](#renderinheritance) | 213,996 | 0% | -71.39% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 238,020 | 0% | -73.13% | 28 | 0% | -93.64% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 27,520 | 0% | -63.37% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 82,596 | 0% | -63.77% | 31 | 0% | -83.33% | 3,912 | 0% | -37.07% |
| [Render/macros](#rendermacros) | 1,020,158 | 0% | -60.20% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,489,901 | 0% | -69.46% | 958 | 0% | -97.20% | 476,112 | 0% | -54.88% |
| [Render/many_tags](#rendermany_tags) | 1,268,450 | 0% | -39.10% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 82,914 | 0% | -65.10% | 28 | 0% | -84.78% | 3,912 | 0% | -35.49% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,643,384 | 0% | -46.23% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 102,293 | +0.01% |  | 63 | 0% |  | 6,096 | 0% | -26.23% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,014,359 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,042 | 0% | -58.23% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 105,515 | 0% | -63.85% | 34 | 0% | -84.82% | 4,792 | 0% | -46.99% |
| [Render/strings](#renderstrings) | 1,062,811 | 0% | -49.33% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,754 | 0% | -63.98% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,672 | 0% | -69.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,336 | -0.37% | 1,469 | +0.34% | 0 |  | 8,902 | -0.72% |
| Render/chat_llama | 1,263 | -0.08% | 492 | +5.81% | 0 |  | 4,866 | -14.50% |
| Load/chat_mistral | 1,958 | +0.93% | 2,128 | -0.23% | 0 |  | 10,502 | +0.90% |
| Render/chat_mistral | 2,409 | +2.08% | 745 | +9.24% | 0 |  | 14,000 | -5.92% |
| Load/chat_qwen | 859 | -0.35% | 1,265 | -0.16% | 0 |  | 5,735 | -0.47% |
| Render/chat_qwen | 1,169 | +2.45% | 364 | +15.92% | 0 |  | 4,193 | -5.78% |
| Load/config_file | 388 | +0.78% | 539 | -1.10% | 0 |  | 3,584 | -1.43% |
| Render/config_file | 7,258 | +2.02% | 2,009 | +1.88% | 0 |  | 69,338 | -4.05% |
| Load/dict_ops | 98 | +1.03% | 78 | 0% | 0 |  | 1,726 | -0.52% |
| Render/dict_ops | 1,014 | -0.20% | 622 | +0.16% | 0 |  | 1,108 | +0.27% |
| Load/expressions | 136 | +5.43% | 213 | +0.95% | 0 |  | 1,563 | -0.57% |
| Render/expressions | 7 | 0% | 4 | 0% | 0 |  | 435 | -1.14% |
| Load/filters | 215 | +5.91% | 149 | +1.36% | 0 |  | 2,344 | -1.64% |
| Render/filters | 93 | +2.20% | 60 | 0% | 0 |  | 1,163 | -1.86% |
| Load/for_filter_if | 40 | -23.08% | 29 | -9.38% | 0 |  | 1,287 | +3.46% |
| Render/for_filter_if | 1,304 | 0% | 55 | 0% | 0 |  | 429 | -31.25% |
| Load/for_loop_vars | 68 | +13.33% | 70 | -1.41% | 0 |  | 1,380 | -2.75% |
| Render/for_loop_vars | 14 | -6.67% | 6 | -14.29% | 0 |  | 263 | -0.38% |
| Load/for_range | 21 | +5.00% | 20 | +5.26% | 0 |  | 1,032 | -0.19% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 223 | -9.35% |
| Load/html_autoescape | 765 | -0.13% | 908 | 0% | 0 |  | 4,898 | +2.94% |
| Render/html_autoescape | 4,162 | -8.18% | 1,539 | -5.35% | 0 |  | 50,756 | -2.02% |
| Load/inheritance | 18 | +28.57% | 17 | +13.33% | 0 |  | 1,021 | -2.85% |
| Render/inheritance | 510 | +1.80% | 98 | +1.03% | 0 |  | 1,521 | +95.25% |
| Load/large_static | 2,617 | +0.08% | 1,198 | 0% | 0 |  | 419 | -7.30% |
| Render/large_static | 724 | 0% | 582 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 215 | -2.71% | 262 | +1.16% | 0 |  | 2,040 | -1.07% |
| Render/macros | 709 | 0% | 212 | 0% | 0 |  | 9,689 | -36.79% |
| Load/many_tags | 39,615 | +2.01% | 31,044 | +1.83% | 0 |  | 302,948 | +3.33% |
| Render/many_tags | 7,485 | -0.01% | 1,720 | +0.06% | 0 |  | 505 | -8.51% |
| Load/mitsuhiko_table | 107 | +2.88% | 204 | 0% | 0 |  | 1,424 | -2.93% |
| Render/mitsuhiko_table | 8,022 | +0.01% | 5,011 | 0% | 0 |  | 461 | -47.25% |
| Load/mitsuhiko_table_wide | 218 | -1.80% | 331 | +1.22% | 0 |  | 1,992 | +4.57% |
| Render/mitsuhiko_table_wide | 8,084 | +0.04% | 21,461 | +0.01% | 0 |  | 475 | -90.07% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 41 | +583.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | +50.00% |
| Load/strings | 263 | -0.38% | 280 | -1.06% | 0 |  | 3,304 | +0.55% |
| Render/strings | 930 | +0.11% | 1,351 | +0.37% | 0 |  | 1,957 | -58.07% |
| Load/substitute | 4 | +33.33% | 7 | 0% | 0 |  | 365 | -9.65% |
| Render/substitute | 0 |  | 0 | -100.00% | 0 |  | 3 | 0% |

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
