# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (39 so far, latest [a9ef179](https://github.com/jinja2cpp/Jinja2Cpp/commit/a9ef179b2d83f0bf1515bb667ab421f5f229bcd7) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 659,254 | -1.39% |  | 380 | -5.71% |  | 39,096 | -0.91% | +0.04% |
| [Render/chat_llama](#renderchat_llama) | 426,101 | -0.27% |  | 349 | 0% |  | 13,320 | 0% | -7.24% |
| [Load/chat_mistral](#loadchat_mistral) | 769,440 | -1.33% |  | 436 | -5.01% |  | 44,968 | -0.97% | +0.72% |
| [Render/chat_mistral](#renderchat_mistral) | 746,115 | -0.18% |  | 502 | 0% |  | 11,704 | -0.41% | -11.39% |
| [Load/chat_qwen](#loadchat_qwen) | 493,651 | -1.49% |  | 289 | -7.37% |  | 27,472 | -1.29% | +0.53% |
| [Render/chat_qwen](#renderchat_qwen) | 421,768 | -0.39% |  | 359 | 0% |  | 7,464 | 0% | -12.23% |
| [Load/config_file](#loadconfig_file) | 164,261 | -5.47% |  | 110 | -17.29% |  | 10,224 | -3.84% | -3.18% |
| [Render/config_file](#renderconfig_file) | 2,142,443 | -0.06% |  | 1,845 | 0% |  | 21,464 | -0.15% | -9.85% |
| [Load/dict_ops](#loaddict_ops) | 53,382 | -14.97% | -63.88% | 41 | -32.79% | -68.70% | 3,888 | -9.83% | -8.65% |
| [Render/dict_ops](#renderdict_ops) | 415,100 | +0.24% | -39.91% | 317 | 0% | -29.56% | 34,136 | +0.05% | -1.61% |
| [Load/expressions](#loadexpressions) | 76,024 | -10.01% | -63.12% | 62 | -24.39% | -57.53% | 5,864 | -6.98% | -4.43% |
| [Render/expressions](#renderexpressions) | 556,212 | +0.02% | -38.62% | 4 | 0% | -86.67% | 2,576 | 0% | -27.31% |
| [Load/filters](#loadfilters) | 118,360 | -8.05% | -56.87% | 94 | -17.54% | -58.77% | 11,032 | -3.97% | -3.36% |
| [Render/filters](#renderfilters) | 63,196 | +0.05% | -28.73% | 47 | 0% | -36.49% | 4,336 | 0% | -11.73% |
| [Load/for_filter_if](#loadfor_filter_if) | 43,704 | -17.08% | -67.72% | 36 | -35.71% | -70.73% | 3,080 | -12.10% | -10.67% |
| [Render/for_filter_if](#renderfor_filter_if) | 518,811 | -0.08% | -40.06% | 207 | 0% | -8.41% | 2,656 | 0% | -27.03% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 60,773 | -11.43% | -66.36% | 48 | -28.36% | -69.43% | 4,136 | -9.30% | -8.50% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 615,834 | -0.48% | -42.59% | 503 | 0% | -3.64% | 1,728 | 0% | -37.57% |
| [Load/for_range](#loadfor_range) | 20,945 | -25.55% | -63.05% | 20 | -45.95% | -64.29% | 1,616 | -20.78% | -19.20% |
| [Render/for_range](#renderfor_range) | 51,705 | 0% | -43.93% | 4 | 0% | -85.19% | 544 | 0% | -64.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 347,707 | -2.94% |  | 251 | -8.39% |  | 24,600 | -1.63% | -0.32% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,832,941 | +0.51% |  | 1,074 | 0% |  | 38,688 | 0% | -4.20% |
| [Load/inheritance](#loadinheritance) | 37,765 | -17.11% | -66.91% | 32 | -37.25% | -70.09% | 2,488 | -14.56% | -13.37% |
| [Render/inheritance](#renderinheritance) | 281,614 | -0.05% | -62.34% | 155 | 0% | -75.08% | 3,824 | 0% | -36.44% |
| [Load/large_static](#loadlarge_static) | 295,098 | -2.25% | -66.69% | 177 | -8.29% | -59.77% | 52,952 | -0.79% | -0.81% |
| [Render/large_static](#renderlarge_static) | 31,752 | -0.11% | -57.74% | 2 | 0% | -88.89% | 36,656 | 0% | -1.55% |
| [Load/macros](#loadmacros) | 88,294 | -9.51% | -61.27% | 72 | -24.21% | -61.29% | 5,848 | -6.76% | -5.92% |
| [Render/macros](#rendermacros) | 1,178,229 | +0.08% | -54.03% | 408 | 0% | -85.05% | 13,680 | 0% | -7.07% |
| [Load/many_tags](#loadmany_tags) | 16,461,752 | +0.01% | -62.73% | 9,637 | -0.23% | -71.86% | 1,059,656 | -0.04% | +0.42% |
| [Render/many_tags](#rendermany_tags) | 1,285,397 | 0% | -38.29% | 3 | 0% | -85.71% | 5,736 | 0% | -9.13% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 87,993 | -10.29% | -62.96% | 72 | -24.21% | -60.87% | 5,712 | -6.91% | -5.80% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 7,265,475 | 0% | -41.20% | 1,026 | 0% | -74.69% | 345,968 | 0% | -0.30% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 110,583 | -12.07% |  | 107 | -18.94% |  | 7,896 | -5.10% | -4.45% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,616,551 | 0% |  | 1,047 | 0% |  | 1,379,968 | 0% | -0.07% |
| [Load/plain_text](#loadplain_text) | 4,275 | -52.53% | -55.82% | 8 | -50.00% | -55.56% | 720 | -37.06% | -35.71% |
| [Render/plain_text](#renderplain_text) | 2,334 | +0.04% | -65.65% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |
| [Load/strings](#loadstrings) | 112,968 | -8.84% | -61.29% | 84 | -20.75% | -62.50% | 8,944 | -4.53% | -1.06% |
| [Render/strings](#renderstrings) | 1,113,544 | -0.08% | -46.91% | 1,083 | 0% | -48.77% | 17,352 | 0% | -3.21% |
| [Load/substitute](#loadsubstitute) | 11,627 | -32.18% | -61.06% | 15 | -44.44% | -59.46% | 1,120 | -27.46% | -26.32% |
| [Render/substitute](#rendersubstitute) | 4,104 | +0.02% | -53.44% | 2 | 0% | -83.33% | 144 | 0% | -80.00% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,962 | -3.06% | 1,375 | -3.91% | 0 |  | 10,286 | +3.00% |
| Render/chat_llama | 1,885 | -1.05% | 510 | -6.42% | 0 |  | 5,277 | +2.11% |
| Load/chat_mistral | 2,294 | -4.58% | 1,801 | -2.60% | 0 |  | 11,793 | +3.65% |
| Render/chat_mistral | 3,840 | +7.23% | 822 | -3.63% | 0 |  | 13,911 | -2.86% |
| Load/chat_qwen | 1,408 | +1.29% | 1,273 | -1.47% | 0 |  | 6,564 | +5.33% |
| Render/chat_qwen | 2,038 | -2.44% | 381 | -3.79% | 0 |  | 4,610 | -0.54% |
| Load/config_file | 642 | -3.31% | 569 | -1.39% | 0 |  | 3,817 | -3.81% |
| Render/config_file | 13,040 | +5.59% | 2,592 | +4.10% | 0 |  | 77,167 | -1.76% |
| Load/dict_ops | 152 | -19.15% | 108 | -15.62% | 0 |  | 1,610 | -6.40% |
| Render/dict_ops | 1,103 | -1.25% | 630 | -1.56% | 0 |  | 1,284 | 0% |
| Load/expressions | 220 | -13.39% | 182 | -16.89% | 0 |  | 1,440 | -7.28% |
| Render/expressions | 38 | +22.58% | 17 | +30.77% | 0 |  | 577 | -3.51% |
| Load/filters | 429 | -11.18% | 248 | -12.98% | 0 |  | 3,131 | -9.06% |
| Render/filters | 218 | +3.32% | 96 | -6.80% | 0 |  | 1,374 | +0.15% |
| Load/for_filter_if | 93 | -27.91% | 54 | -18.18% | 0 |  | 1,063 | -10.90% |
| Render/for_filter_if | 1,330 | -1.48% | 58 | -9.38% | 0 |  | 1,290 | +129.95% |
| Load/for_loop_vars | 118 | -20.27% | 86 | -31.20% | 0 |  | 1,261 | -6.94% |
| Render/for_loop_vars | 49 | -58.47% | 9 | -47.06% | 0 |  | 2,093 | -41.44% |
| Load/for_range | 33 | -23.26% | 30 | -33.33% | 0 |  | 826 | -7.50% |
| Render/for_range | 3 | -50.00% | 2 | -50.00% | 0 |  | 353 | -4.08% |
| Load/html_autoescape | 1,096 | -6.08% | 817 | -4.78% | 0 |  | 5,664 | +2.87% |
| Render/html_autoescape | 16,366 | +44.60% | 2,617 | -0.27% | 0 |  | 63,034 | +5.50% |
| Load/inheritance | 38 | +15.15% | 34 | +61.90% | 0 |  | 861 | -14.75% |
| Render/inheritance | 629 | -28.20% | 129 | -11.64% | 0 |  | 2,217 | +17.86% |
| Load/large_static | 2,852 | +0.18% | 1,230 | -2.30% | 0 |  | 489 | -12.37% |
| Render/large_static | 915 | -0.54% | 580 | 0% | 0 |  | 53 | +140.91% |
| Load/macros | 289 | -3.34% | 263 | -4.36% | 0 |  | 1,841 | -1.76% |
| Render/macros | 706 | -0.56% | 207 | +0.98% | 0 |  | 8,287 | -10.31% |
| Load/many_tags | 58,068 | +2.08% | 29,755 | +1.41% | 0 |  | 339,907 | +0.74% |
| Render/many_tags | 13,545 | -0.65% | 1,669 | +0.48% | 0 |  | 4,503 | +502.01% |
| Load/mitsuhiko_table | 181 | -17.73% | 205 | -12.02% | 0 |  | 1,149 | -4.57% |
| Render/mitsuhiko_table | 8,096 | +0.17% | 5,189 | +0.02% | 0 |  | 680 | +3.82% |
| Load/mitsuhiko_table_wide | 347 | -11.93% | 379 | -2.07% | 0 |  | 1,598 | -11.57% |
| Render/mitsuhiko_table_wide | 8,216 | -2.07% | 21,472 | +0.04% | 0 |  | 12,943 | +1725.53% |
| Load/plain_text | 3 |  | 4 |  | 0 |  | 11 | -67.65% |
| Render/plain_text | 1 |  | 0 |  | 0 |  | 16 | +45.45% |
| Load/strings | 416 | -8.17% | 292 | -17.05% | 0 |  | 3,456 | -4.19% |
| Render/strings | 968 | -2.71% | 1,389 | -0.71% | 0 |  | 3,263 | +37.45% |
| Load/substitute | 6 | -14.29% | 10 | +42.86% | 0 |  | 256 | -43.49% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 14 | +100.00% |

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
