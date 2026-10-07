# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (48 so far, latest [d333c43](https://github.com/jinja2cpp/Jinja2Cpp/commit/d333c438df34802917547a4b512e654e4297c1ee) on 2026-10-07). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 578,425 | +0.23% |  | 137 | 0% |  | 23,752 | 0% | -39.22% |
| [Render/chat_llama](#renderchat_llama) | 410,831 | -0.01% |  | 348 | 0% |  | 13,232 | 0% | -7.86% |
| [Load/chat_mistral](#loadchat_mistral) | 676,074 | +0.20% |  | 127 | 0% |  | 29,424 | 0% | -34.10% |
| [Render/chat_mistral](#renderchat_mistral) | 712,637 | 0% |  | 475 | 0% |  | 11,664 | 0% | -11.69% |
| [Load/chat_qwen](#loadchat_qwen) | 429,773 | +0.18% |  | 98 | 0% |  | 18,496 | 0% | -32.32% |
| [Render/chat_qwen](#renderchat_qwen) | 397,948 | -0.02% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 148,554 | +0.19% |  | 43 | 0% |  | 6,816 | 0% | -35.45% |
| [Render/config_file](#renderconfig_file) | 1,999,681 | -0.19% |  | 1,703 | -0.06% |  | 21,528 | +0.19% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 52,871 | +0.14% | -64.22% | 19 | 0% | -85.50% | 2,888 | 0% | -32.14% |
| [Render/dict_ops](#renderdict_ops) | 403,303 | 0% | -41.62% | 315 | 0% | -30.00% | 34,096 | 0% | -1.73% |
| [Load/expressions](#loadexpressions) | 70,619 | +0.13% | -65.74% | 24 | 0% | -83.56% | 4,392 | 0% | -28.42% |
| [Render/expressions](#renderexpressions) | 532,175 | -0.01% | -41.27% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 99,818 | +0.22% | -63.63% | 46 | 0% | -79.82% | 4,792 | 0% | -58.02% |
| [Render/filters](#renderfilters) | 59,915 | -0.08% | -32.43% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 43,334 | +0.10% | -67.99% | 15 | 0% | -87.80% | 2,304 | 0% | -33.18% |
| [Render/for_filter_if](#renderfor_filter_if) | 463,830 | -0.01% | -46.41% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 58,205 | +0.13% | -67.78% | 17 | 0% | -89.17% | 2,904 | 0% | -35.75% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 276,234 | -0.02% | -74.25% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 21,557 | +0.12% | -61.97% | 12 | 0% | -78.57% | 1,416 | 0% | -29.20% |
| [Render/for_range](#renderfor_range) | 46,749 | -0.10% | -49.30% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 291,785 | +0.19% |  | 76 | 0% |  | 16,072 | 0% | -34.88% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,683,939 | -0.05% |  | 1,011 | -0.10% |  | 38,752 | +0.10% | -4.04% |
| [Load/inheritance](#loadinheritance) | 37,954 | +0.13% | -66.74% | 17 | 0% | -84.11% | 1,960 | 0% | -31.75% |
| [Render/inheritance](#renderinheritance) | 213,410 | -0.93% | -71.46% | 76 | -2.56% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 238,542 | +0.30% | -73.08% | 29 | 0% | -93.41% | 45,312 | 0% | -15.12% |
| [Render/large_static](#renderlarge_static) | 27,339 | -0.18% | -63.61% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,528 | +0.17% | -64.23% | 34 | 0% | -81.72% | 4,512 | 0% | -27.41% |
| [Render/macros](#rendermacros) | 1,175,116 | 0% | -54.15% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,703,952 | +0.25% | -68.97% | 2,158 | 0% | -93.70% | 561,936 | 0% | -46.75% |
| [Render/many_tags](#rendermany_tags) | 1,268,715 | 0% | -39.09% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 83,516 | +0.09% | -64.84% | 32 | 0% | -82.61% | 4,328 | 0% | -28.63% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,653,372 | 0% | -46.15% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 102,895 | +0.28% |  | 67 | 0% |  | 6,512 | 0% | -21.20% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,024,504 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,572 | -0.15% | -52.75% | 6 | 0% | -66.67% | 752 | 0% | -32.86% |
| [Render/plain_text](#renderplain_text) | 986 | -4.64% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 105,492 | +0.22% | -63.85% | 43 | 0% | -80.80% | 5,512 | 0% | -39.03% |
| [Render/strings](#renderstrings) | 1,063,235 | -0.01% | -49.31% | 976 | 0% | -53.83% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,312 | +0.21% | -62.11% | 9 | 0% | -75.68% | 960 | 0% | -36.84% |
| [Render/substitute](#rendersubstitute) | 2,666 | -1.77% | -69.76% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,683 | -1.75% | 1,615 | -1.76% | 0 |  | 9,080 | +5.35% |
| Render/chat_llama | 1,298 | -1.29% | 475 | +1.50% | 0 |  | 4,887 | -1.53% |
| Load/chat_mistral | 2,320 | -1.69% | 2,266 | -1.35% | 0 |  | 10,370 | +3.90% |
| Render/chat_mistral | 2,585 | +2.54% | 695 | +0.14% | 0 |  | 14,541 | +0.12% |
| Load/chat_qwen | 1,031 | +1.18% | 1,318 | -0.15% | 0 |  | 5,451 | +2.50% |
| Render/chat_qwen | 1,237 | -0.48% | 325 | +0.62% | 0 |  | 4,414 | +1.12% |
| Load/config_file | 415 | -3.04% | 579 | +0.70% | 0 |  | 3,343 | +0.72% |
| Render/config_file | 9,685 | +8.93% | 2,642 | +6.23% | 0 |  | 73,021 | -1.10% |
| Load/dict_ops | 115 | -5.74% | 114 | +0.88% | 0 |  | 1,681 | +0.30% |
| Render/dict_ops | 1,073 | -0.37% | 607 | -0.49% | 0 |  | 1,155 | +3.03% |
| Load/expressions | 155 | -5.49% | 227 | -5.02% | 0 |  | 1,566 | +1.56% |
| Render/expressions | 4 | 0% | 2 | 0% | 0 |  | 458 | +0.66% |
| Load/filters | 229 | -6.91% | 171 | 0% | 0 |  | 2,287 | +10.38% |
| Render/filters | 142 | +1.43% | 71 | -10.13% | 0 |  | 1,164 | -2.68% |
| Load/for_filter_if | 63 | +18.87% | 46 | +21.05% | 0 |  | 1,168 | -0.26% |
| Render/for_filter_if | 1,240 | -0.48% | 54 | +8.00% | 0 |  | 445 | -63.19% |
| Load/for_loop_vars | 91 | +7.06% | 101 | 0% | 0 |  | 1,332 | +0.38% |
| Render/for_loop_vars | 10 | +25.00% | 5 | +150.00% | 0 |  | 259 | -18.30% |
| Load/for_range | 35 | +6.06% | 30 | +3.45% | 0 |  | 1,009 | +1.71% |
| Render/for_range | 8 | +166.67% | 3 | +50.00% | 0 |  | 246 | -15.17% |
| Load/html_autoescape | 854 | -0.70% | 950 | -0.52% | 0 |  | 4,501 | +3.73% |
| Render/html_autoescape | 5,149 | -8.74% | 1,212 | -26.41% | 0 |  | 53,050 | -3.32% |
| Load/inheritance | 23 | -20.69% | 23 | -11.54% | 0 |  | 1,072 | +2.78% |
| Render/inheritance | 557 | +2.58% | 104 | +7.22% | 0 |  | 816 | -43.10% |
| Load/large_static | 2,625 | +0.04% | 1,259 | 0% | 0 |  | 484 | +5.22% |
| Render/large_static | 749 | -0.66% | 579 | +0.70% | 0 |  | 10 | -16.67% |
| Load/macros | 209 | +8.29% | 242 | +1.26% | 0 |  | 1,884 | -0.11% |
| Render/macros | 1,006 | +40.31% | 342 | +50.66% | 0 |  | 8,376 | -32.64% |
| Load/many_tags | 47,820 | +1.30% | 35,581 | +1.85% | 0 |  | 295,078 | +6.63% |
| Render/many_tags | 8,700 | +0.03% | 1,809 | -0.39% | 0 |  | 2,107 | -60.82% |
| Load/mitsuhiko_table | 116 | -10.08% | 200 | -6.54% | 0 |  | 1,411 | +0.93% |
| Render/mitsuhiko_table | 8,041 | -0.01% | 5,122 | +0.08% | 0 |  | 467 | -52.73% |
| Load/mitsuhiko_table_wide | 225 | +0.45% | 368 | -0.54% | 0 |  | 1,969 | +5.63% |
| Render/mitsuhiko_table_wide | 8,119 | +0.14% | 21,521 | +0.01% | 0 |  | 516 | +5.09% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 8 | -69.23% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 5 | -16.67% |
| Load/strings | 293 | -5.18% | 299 | -1.64% | 0 |  | 3,084 | +3.52% |
| Render/strings | 946 | +1.39% | 1,323 | +0.38% | 0 |  | 2,442 | +18.77% |
| Load/substitute | 4 | -20.00% | 6 | 0% | 0 |  | 381 | +5.54% |
| Render/substitute | 1 |  | 0 | -100.00% | 0 |  | 4 | -20.00% |

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
