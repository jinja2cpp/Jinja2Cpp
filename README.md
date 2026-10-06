# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (44 so far, latest [20bd8ae](https://github.com/jinja2cpp/Jinja2Cpp/commit/20bd8aeb378cba23a6d795dbc6a0e6cbc78ec0cc) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 528,078 | -18.71% |  | 129 | -66.92% |  | 39,000 | +25.68% | -0.20% |
| [Render/chat_llama](#renderchat_llama) | 416,430 | -0.58% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 600,488 | -20.79% |  | 116 | -73.76% |  | 38,560 | +0.12% | -13.64% |
| [Render/chat_mistral](#renderchat_mistral) | 736,784 | -0.35% |  | 503 | 0% |  | 11,632 | +0.14% | -11.93% |
| [Load/chat_qwen](#loadchat_qwen) | 387,534 | -19.76% |  | 94 | -67.70% |  | 36,976 | +50.75% | +35.30% |
| [Render/chat_qwen](#renderchat_qwen) | 412,393 | -0.52% |  | 358 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 130,357 | -19.35% |  | 41 | -64.66% |  | 9,208 | +1.77% | -12.80% |
| [Render/config_file](#renderconfig_file) | 2,009,314 | -0.36% |  | 1,770 | 0% |  | 21,448 | +0.30% | -9.91% |
| [Load/dict_ops](#loaddict_ops) | 45,580 | -11.67% | -69.16% | 19 | -54.76% | -85.50% | 4,632 | +37.86% | +8.83% |
| [Render/dict_ops](#renderdict_ops) | 404,684 | +0.22% | -41.42% | 316 | 0% | -29.78% | 34,048 | +0.09% | -1.87% |
| [Load/expressions](#loadexpressions) | 61,615 | -16.73% | -70.11% | 24 | -61.29% | -83.56% | 8,976 | +67.21% | +46.28% |
| [Render/expressions](#renderexpressions) | 530,929 | -0.09% | -41.41% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 92,341 | -21.73% | -66.35% | 46 | -56.60% | -79.82% | 9,504 | +56.32% | -16.75% |
| [Render/filters](#renderfilters) | 59,806 | -0.28% | -32.55% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 36,679 | -12.74% | -72.91% | 15 | -58.33% | -87.80% | 4,336 | +43.77% | +25.75% |
| [Render/for_filter_if](#renderfor_filter_if) | 515,669 | -0.11% | -40.42% | 206 | 0% | -8.85% | 2,552 | 0% | -29.89% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 49,970 | -15.68% | -72.34% | 17 | -64.58% | -89.17% | 4,408 | +12.22% | -2.48% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 600,406 | -0.68% | -44.03% | 502 | 0% | -3.83% | 1,624 | 0% | -41.33% |
| [Load/for_range](#loadfor_range) | 18,698 | -8.62% | -67.01% | 12 | -40.00% | -78.57% | 2,224 | +34.95% | +11.20% |
| [Render/for_range](#renderfor_range) | 47,334 | -0.21% | -48.67% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 254,838 | -24.85% |  | 73 | -71.26% |  | 35,976 | +72.56% | +45.77% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,768,724 | -0.17% |  | 1,072 | 0% |  | 38,680 | +0.04% | -4.22% |
| [Load/inheritance](#loadinheritance) | 33,992 | -9.53% | -70.21% | 17 | -48.48% | -84.11% | 2,528 | -1.56% | -11.98% |
| [Render/inheritance](#renderinheritance) | 202,734 | +0.50% | -72.89% | 78 | 0% | -87.46% | 3,264 | +0.49% | -45.74% |
| [Load/large_static](#loadlarge_static) | 218,623 | -23.95% | -75.32% | 28 | -84.27% | -93.64% | 53,352 | +10.07% | -0.06% |
| [Render/large_static](#renderlarge_static) | 26,629 | -0.38% | -64.56% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 72,844 | -15.38% | -68.04% | 34 | -52.78% | -81.72% | 9,368 | +71.45% | +50.71% |
| [Render/macros](#rendermacros) | 1,150,403 | +0.08% | -55.12% | 407 | 0% | -85.09% | 13,576 | 0% | -7.77% |
| [Load/many_tags](#loadmany_tags) | 12,045,926 | -25.06% | -72.72% | 2,155 | -78.95% | -93.71% | 801,528 | +3.50% | -24.04% |
| [Render/many_tags](#rendermany_tags) | 1,265,428 | -0.06% | -39.25% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 74,437 | -14.79% | -68.67% | 32 | -55.56% | -82.61% | 9,136 | +69.44% | +50.66% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,973,771 | -0.16% | -43.56% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 93,936 | -13.06% |  | 67 | -37.38% |  | 11,320 | +49.42% | +36.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,324,773 | -0.15% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 3,833 | -6.76% | -60.39% | 6 | -25.00% | -66.67% | 1,152 | +35.85% | +2.86% |
| [Render/plain_text](#renderplain_text) | 994 | -0.10% | -85.37% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 95,574 | -13.82% | -67.25% | 43 | -52.22% | -80.80% | 9,712 | +47.87% | +7.43% |
| [Render/strings](#renderstrings) | 1,104,807 | -0.06% | -47.33% | 1,084 | 0% | -48.72% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,226 | -7.82% | -65.75% | 10 | -33.33% | -72.97% | 2,200 | +102.21% | +44.74% |
| [Render/substitute](#rendersubstitute) | 2,614 | -0.04% | -70.35% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,143 | -32.17% | 1,218 | +0.33% | 0 |  | 8,220 | -14.14% |
| Render/chat_llama | 1,596 | -8.70% | 499 | +4.61% | 0 |  | 5,179 | -5.34% |
| Load/chat_mistral | 1,312 | -36.06% | 1,572 | -5.53% | 0 |  | 8,924 | -18.14% |
| Render/chat_mistral | 3,252 | -6.55% | 834 | +8.74% | 0 |  | 14,162 | -6.62% |
| Load/chat_qwen | 837 | -34.51% | 1,125 | -7.64% | 0 |  | 5,021 | -17.97% |
| Render/chat_qwen | 1,750 | -1.74% | 382 | +7.30% | 0 |  | 4,601 | -3.34% |
| Load/config_file | 366 | -37.86% | 449 | -15.28% | 0 |  | 2,964 | -18.50% |
| Render/config_file | 11,635 | -5.97% | 2,617 | -0.23% | 0 |  | 68,296 | -3.99% |
| Load/dict_ops | 92 | -22.69% | 54 | -23.94% | 0 |  | 1,409 | -5.88% |
| Render/dict_ops | 1,168 | +5.89% | 624 | +0.97% | 0 |  | 1,173 | -1.18% |
| Load/expressions | 113 | -38.59% | 121 | -26.22% | 0 |  | 1,375 | -5.17% |
| Render/expressions | 10 | +25.00% | 3 | 0% | 0 |  | 479 | +6.21% |
| Load/filters | 210 | -39.66% | 131 | -12.67% | 0 |  | 2,297 | -10.10% |
| Render/filters | 167 | -5.11% | 91 | +13.75% | 0 |  | 1,172 | +1.74% |
| Load/for_filter_if | 38 | -42.42% | 21 | -38.24% | 0 |  | 971 | -5.54% |
| Render/for_filter_if | 1,243 | -9.67% | 51 | -12.07% | 0 |  | 416 | -65.62% |
| Load/for_loop_vars | 53 | -45.92% | 54 | -29.87% | 0 |  | 1,087 | -12.06% |
| Render/for_loop_vars | 71 | -26.04% | 20 | +122.22% | 0 |  | 2,935 | -27.62% |
| Load/for_range | 20 | -39.39% | 18 | -33.33% | 0 |  | 786 | -4.26% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 221 | +18.18% |
| Load/html_autoescape | 692 | -33.72% | 796 | +1.14% | 0 |  | 4,150 | -21.18% |
| Render/html_autoescape | 11,881 | -6.21% | 2,471 | -12.34% | 0 |  | 60,359 | -0.91% |
| Load/inheritance | 22 | -40.54% | 23 | -4.17% | 0 |  | 812 | -8.66% |
| Render/inheritance | 543 | -0.91% | 99 | -2.94% | 0 |  | 1,460 | +87.42% |
| Load/large_static | 2,599 | -5.80% | 1,178 | -2.56% | 0 |  | 417 | -18.24% |
| Render/large_static | 820 | -1.20% | 576 | 0% | 0 |  | 11 | +175.00% |
| Load/macros | 149 | -41.80% | 136 | -44.26% | 0 |  | 1,653 | -10.75% |
| Render/macros | 667 | -1.19% | 223 | 0% | 0 |  | 10,017 | +6.17% |
| Load/many_tags | 25,231 | -49.12% | 22,408 | -3.06% | 0 |  | 280,346 | -9.89% |
| Render/many_tags | 11,562 | -2.00% | 1,847 | -0.81% | 0 |  | 6,387 | +18.08% |
| Load/mitsuhiko_table | 71 | -57.49% | 100 | -49.24% | 0 |  | 1,055 | -12.01% |
| Render/mitsuhiko_table | 8,047 | -0.24% | 5,121 | -0.04% | 0 |  | 541 | -97.79% |
| Load/mitsuhiko_table_wide | 180 | -40.59% | 270 | -19.40% | 0 |  | 1,459 | -13.36% |
| Render/mitsuhiko_table_wide | 8,154 | +0.30% | 21,517 | 0% | 0 |  | 4,932 | -85.30% |
| Load/plain_text | 0 |  | 0 |  | 0 |  | 7 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 3 | 0% |
| Load/strings | 278 | -25.67% | 212 | -16.21% | 0 |  | 2,693 | -11.27% |
| Render/strings | 1,017 | +9.35% | 1,340 | +0.15% | 0 |  | 5,685 | +19.99% |
| Load/substitute | 3 | -25.00% | 5 | +25.00% | 0 |  | 298 | +12.45% |
| Render/substitute | 1 |  | 0 |  | 0 |  | 2 | -50.00% |

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
