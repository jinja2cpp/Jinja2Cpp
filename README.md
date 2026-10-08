# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (55 so far, latest [6388fc9](https://github.com/jinja2cpp/Jinja2Cpp/commit/6388fc9b8fcc3b611e33bf85213f5bbe717276c0) on 2026-10-08). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 565,707 | -2.74% |  | 92 | -8.91% |  | 20,928 | -9.92% | -46.45% |
| [Render/chat_llama](#renderchat_llama) | 409,998 | -0.34% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 668,821 | -2.04% |  | 74 | -13.95% |  | 25,288 | -12.90% | -43.36% |
| [Render/chat_mistral](#renderchat_mistral) | 709,322 | -0.33% |  | 474 | 0% |  | 11,632 | 0% | -11.93% |
| [Load/chat_qwen](#loadchat_qwen) | 427,703 | -1.98% |  | 68 | -12.82% |  | 16,256 | -11.38% | -40.52% |
| [Render/chat_qwen](#renderchat_qwen) | 396,188 | -0.34% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 148,831 | -1.02% |  | 34 | 0% |  | 5,928 | -8.18% | -43.86% |
| [Render/config_file](#renderconfig_file) | 1,914,055 | +0.04% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 52,401 | -1.63% | -64.54% | 15 | 0% | -88.55% | 2,520 | -8.43% | -40.79% |
| [Render/dict_ops](#renderdict_ops) | 400,714 | -0.10% | -42.00% | 315 | 0% | -30.00% | 34,032 | 0% | -1.91% |
| [Load/expressions](#loadexpressions) | 68,207 | -4.79% | -66.91% | 18 | -14.29% | -87.67% | 3,656 | -15.68% | -40.42% |
| [Render/expressions](#renderexpressions) | 518,965 | -2.52% | -42.73% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,407 | -2.21% | -64.51% | 32 | 0% | -85.96% | 3,960 | -6.07% | -65.31% |
| [Render/filters](#renderfilters) | 60,231 | +0.26% | -32.07% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 41,856 | -1.45% | -69.08% | 9 | -10.00% | -92.68% | 2,032 | -6.62% | -41.07% |
| [Render/for_filter_if](#renderfor_filter_if) | 461,272 | -0.57% | -46.70% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 57,874 | -0.97% | -67.97% | 13 | 0% | -91.72% | 2,512 | -9.51% | -44.42% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 275,041 | -0.44% | -74.36% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 20,600 | -3.50% | -63.66% | 10 | 0% | -82.14% | 1,200 | -11.24% | -40.00% |
| [Render/for_range](#renderfor_range) | 46,735 | -0.03% | -49.32% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 290,865 | -2.13% |  | 63 | -3.08% |  | 13,624 | -10.27% | -44.80% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,612,460 | -0.11% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 35,986 | -0.44% | -68.47% | 10 | 0% | -90.65% | 1,616 | -0.98% | -43.73% |
| [Render/inheritance](#renderinheritance) | 213,996 | +0.24% | -71.39% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 238,020 | -1.18% | -73.13% | 28 | 0% | -93.64% | 43,120 | -2.28% | -19.23% |
| [Render/large_static](#renderlarge_static) | 27,520 | +0.55% | -63.37% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 82,596 | -1.49% | -63.77% | 31 | 0% | -83.33% | 3,912 | -11.41% | -37.07% |
| [Render/macros](#rendermacros) | 1,020,158 | +0.28% | -60.20% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,489,516 | -1.84% | -69.46% | 958 | 0% | -97.20% | 476,112 | -10.30% | -54.88% |
| [Render/many_tags](#rendermany_tags) | 1,268,450 | -0.06% | -39.10% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 82,915 | -0.72% | -65.10% | 28 | 0% | -84.78% | 3,912 | -2.78% | -35.49% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,643,384 | -0.15% | -46.23% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 102,284 | -0.60% |  | 63 | 0% |  | 6,096 | -1.80% | -26.23% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,014,359 | -0.14% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,042 | -0.86% | -58.23% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 105,515 | -1.91% | -63.85% | 34 | 0% | -84.82% | 4,792 | -8.69% | -46.99% |
| [Render/strings](#renderstrings) | 1,062,772 | +0.01% | -49.33% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,754 | -1.81% | -63.98% | 8 | 0% | -78.38% | 832 | -5.45% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,672 | +0.23% | -69.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,341 | -16.40% | 1,464 | -11.65% | 0 |  | 8,967 | +2.83% |
| Render/chat_llama | 1,264 | -5.11% | 465 | -2.31% | 0 |  | 5,691 | +19.28% |
| Load/chat_mistral | 1,940 | -16.16% | 2,133 | -7.74% | 0 |  | 10,408 | +4.24% |
| Render/chat_mistral | 2,360 | -6.65% | 682 | -4.75% | 0 |  | 14,881 | +8.46% |
| Load/chat_qwen | 862 | -20.48% | 1,267 | -8.12% | 0 |  | 5,762 | -0.55% |
| Render/chat_qwen | 1,141 | -10.44% | 314 | -5.99% | 0 |  | 4,450 | +6.64% |
| Load/config_file | 385 | -0.77% | 545 | -4.05% | 0 |  | 3,636 | +1.65% |
| Render/config_file | 7,114 | +0.32% | 1,972 | -6.05% | 0 |  | 72,265 | +1.77% |
| Load/dict_ops | 97 | -9.35% | 78 | -19.59% | 0 |  | 1,735 | -2.47% |
| Render/dict_ops | 1,016 | -2.59% | 621 | 0% | 0 |  | 1,105 | -2.99% |
| Load/expressions | 129 | -16.77% | 211 | -12.81% | 0 |  | 1,572 | +1.09% |
| Render/expressions | 7 | -46.15% | 4 | 0% | 0 |  | 440 | -8.90% |
| Load/filters | 203 | -7.73% | 147 | -11.98% | 0 |  | 2,383 | +0.25% |
| Render/filters | 91 | +10.98% | 60 | -18.92% | 0 |  | 1,185 | -1.74% |
| Load/for_filter_if | 52 | +4.00% | 32 | 0% | 0 |  | 1,244 | -2.51% |
| Render/for_filter_if | 1,304 | -0.08% | 55 | +1.85% | 0 |  | 624 | -69.66% |
| Load/for_loop_vars | 60 | -15.49% | 71 | -17.44% | 0 |  | 1,419 | -4.12% |
| Render/for_loop_vars | 15 | +36.36% | 7 | +16.67% | 0 |  | 264 | -4.00% |
| Load/for_range | 20 | -35.48% | 19 | -26.92% | 0 |  | 1,034 | -2.54% |
| Render/for_range | 2 | -50.00% | 1 | -50.00% | 0 |  | 246 | -3.91% |
| Load/html_autoescape | 766 | -8.04% | 908 | -7.63% | 0 |  | 4,758 | -3.29% |
| Render/html_autoescape | 4,533 | -5.23% | 1,626 | -9.42% | 0 |  | 51,805 | -0.68% |
| Load/inheritance | 14 | -33.33% | 15 | -21.05% | 0 |  | 1,051 | +0.67% |
| Render/inheritance | 501 | -1.57% | 97 | -2.02% | 0 |  | 779 | -4.18% |
| Load/large_static | 2,615 | -0.57% | 1,198 | -2.28% | 0 |  | 452 | -2.59% |
| Render/large_static | 724 | -1.76% | 582 | 0% | 0 |  | 4 | 0% |
| Load/macros | 221 | -2.64% | 259 | -9.44% | 0 |  | 2,062 | +2.95% |
| Render/macros | 709 | +4.88% | 212 | -0.47% | 0 |  | 15,328 | -4.40% |
| Load/many_tags | 38,833 | -14.65% | 30,485 | -8.86% | 0 |  | 293,184 | -1.70% |
| Render/many_tags | 7,486 | -8.41% | 1,719 | -3.91% | 0 |  | 552 | -92.56% |
| Load/mitsuhiko_table | 104 | +6.12% | 204 | +2.00% | 0 |  | 1,467 | +3.60% |
| Render/mitsuhiko_table | 8,021 | -0.04% | 5,011 | +0.04% | 0 |  | 874 | +90.00% |
| Load/mitsuhiko_table_wide | 222 | +1.83% | 327 | -4.11% | 0 |  | 1,905 | -4.99% |
| Render/mitsuhiko_table_wide | 8,081 | +0.02% | 21,459 | 0% | 0 |  | 4,783 | +832.36% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -87.23% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 264 | -7.04% | 283 | -7.21% | 0 |  | 3,286 | +0.70% |
| Render/strings | 929 | 0% | 1,346 | +0.52% | 0 |  | 4,667 | +68.24% |
| Load/substitute | 3 | -62.50% | 7 | -30.00% | 0 |  | 404 | -4.27% |
| Render/substitute | 0 |  | 1 |  | 0 |  | 3 | 0% |

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
