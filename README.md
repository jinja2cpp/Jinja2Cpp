# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (43 so far, latest [9736456](https://github.com/jinja2cpp/Jinja2Cpp/commit/9736456f0437f2720e0e050c86d69501a24f54f7) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 649,598 | -0.13% |  | 390 | 0% |  | 31,032 | -0.41% | -20.59% |
| [Render/chat_llama](#renderchat_llama) | 418,873 | -0.41% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 758,094 | -0.37% |  | 442 | 0% |  | 38,512 | -0.25% | -13.74% |
| [Render/chat_mistral](#renderchat_mistral) | 739,391 | -0.21% |  | 503 | 0% |  | 11,616 | 0% | -12.05% |
| [Load/chat_qwen](#loadchat_qwen) | 482,954 | -0.54% |  | 291 | 0% |  | 24,528 | -0.33% | -10.25% |
| [Render/chat_qwen](#renderchat_qwen) | 414,554 | -0.52% |  | 358 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 161,643 | -1.20% |  | 116 | 0% |  | 9,048 | -0.18% | -14.32% |
| [Render/config_file](#renderconfig_file) | 2,016,639 | -0.06% |  | 1,770 | 0% |  | 21,384 | -0.15% | -10.18% |
| [Load/dict_ops](#loaddict_ops) | 51,605 | +0.22% | -65.08% | 42 | 0% | -67.94% | 3,360 | -0.47% | -21.05% |
| [Render/dict_ops](#renderdict_ops) | 403,805 | -0.09% | -41.55% | 316 | 0% | -29.78% | 34,016 | 0% | -1.96% |
| [Load/expressions](#loadexpressions) | 73,994 | -0.04% | -64.11% | 62 | 0% | -57.53% | 5,368 | 0% | -12.52% |
| [Render/expressions](#renderexpressions) | 531,403 | 0% | -41.36% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 117,981 | +0.30% | -57.01% | 106 | 0% | -53.51% | 6,080 | -3.31% | -46.74% |
| [Render/filters](#renderfilters) | 59,971 | -0.17% | -32.37% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 42,035 | -0.54% | -68.95% | 36 | 0% | -70.73% | 3,016 | 0% | -12.53% |
| [Render/for_filter_if](#renderfor_filter_if) | 516,230 | +0.36% | -40.35% | 206 | 0% | -8.85% | 2,552 | 0% | -29.89% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 59,261 | -0.55% | -67.20% | 48 | 0% | -69.43% | 3,928 | 0% | -13.10% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 604,531 | +0.13% | -43.65% | 502 | 0% | -3.83% | 1,624 | 0% | -41.33% |
| [Load/for_range](#loadfor_range) | 20,462 | -0.03% | -63.90% | 20 | 0% | -64.29% | 1,648 | 0% | -17.60% |
| [Render/for_range](#renderfor_range) | 47,435 | +0.02% | -48.56% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 339,123 | -0.92% |  | 254 | 0% |  | 20,848 | -0.31% | -15.53% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,771,661 | -0.02% |  | 1,072 | 0% |  | 38,664 | 0% | -4.26% |
| [Load/inheritance](#loadinheritance) | 37,573 | -0.09% | -67.08% | 33 | 0% | -69.16% | 2,568 | 0% | -10.58% |
| [Render/inheritance](#renderinheritance) | 201,724 | -0.42% | -73.03% | 78 | 0% | -87.46% | 3,248 | 0% | -46.01% |
| [Load/large_static](#loadlarge_static) | 287,491 | 0% | -67.55% | 178 | 0% | -59.55% | 48,472 | 0% | -9.20% |
| [Render/large_static](#renderlarge_static) | 26,730 | 0% | -64.42% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 86,079 | -0.22% | -62.24% | 72 | 0% | -61.29% | 5,464 | 0% | -12.10% |
| [Render/macros](#rendermacros) | 1,149,495 | -0.03% | -55.15% | 407 | 0% | -85.09% | 13,576 | 0% | -7.77% |
| [Load/many_tags](#loadmany_tags) | 16,073,791 | -0.30% | -63.60% | 10,238 | 0% | -70.11% | 774,424 | -1.23% | -26.61% |
| [Render/many_tags](#rendermany_tags) | 1,266,164 | +0.29% | -39.21% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 87,358 | +0.01% | -63.23% | 72 | 0% | -60.87% | 5,392 | 0% | -11.08% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,984,801 | +0.16% | -43.47% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 108,046 | +0.01% |  | 107 | 0% |  | 7,576 | 0% | -8.33% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,335,890 | +0.15% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,111 | -0.24% | -57.51% | 8 | 0% | -55.56% | 848 | 0% | -24.29% |
| [Render/plain_text](#renderplain_text) | 995 | 0% | -85.36% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 110,896 | -1.79% | -62.00% | 90 | 0% | -59.82% | 6,568 | -1.68% | -27.35% |
| [Render/strings](#renderstrings) | 1,105,514 | -0.11% | -47.29% | 1,084 | 0% | -48.72% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,093 | -0.09% | -62.85% | 15 | 0% | -59.46% | 1,088 | 0% | -28.42% |
| [Render/substitute](#rendersubstitute) | 2,615 | 0% | -70.33% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,685 | -6.96% | 1,214 | -2.57% | 0 |  | 9,574 | -0.29% |
| Render/chat_llama | 1,748 | -1.41% | 477 | -1.04% | 0 |  | 5,471 | +14.99% |
| Load/chat_mistral | 2,052 | -5.74% | 1,664 | -3.48% | 0 |  | 10,902 | -1.51% |
| Render/chat_mistral | 3,480 | +0.87% | 767 | +0.79% | 0 |  | 15,166 | +9.95% |
| Load/chat_qwen | 1,278 | -2.89% | 1,218 | +0.74% | 0 |  | 6,121 | +3.99% |
| Render/chat_qwen | 1,781 | -3.00% | 356 | -7.05% | 0 |  | 4,760 | +12.85% |
| Load/config_file | 589 | -6.21% | 530 | -0.56% | 0 |  | 3,637 | -0.41% |
| Render/config_file | 12,374 | +0.03% | 2,623 | +3.15% | 0 |  | 71,134 | +1.05% |
| Load/dict_ops | 119 | -13.77% | 71 | -15.48% | 0 |  | 1,497 | -3.04% |
| Render/dict_ops | 1,103 | +1.01% | 618 | +0.32% | 0 |  | 1,187 | +0.25% |
| Load/expressions | 184 | -7.54% | 164 | -5.20% | 0 |  | 1,450 | +1.12% |
| Render/expressions | 8 | +33.33% | 3 | +200.00% | 0 |  | 451 | +4.40% |
| Load/filters | 348 | +6.75% | 150 | +1.35% | 0 |  | 2,555 | -6.48% |
| Render/filters | 176 | +4.14% | 80 | +1.27% | 0 |  | 1,152 | -0.95% |
| Load/for_filter_if | 66 | -19.51% | 34 | -12.82% | 0 |  | 1,028 | +0.10% |
| Render/for_filter_if | 1,376 | +9.55% | 58 | +11.54% | 0 |  | 1,210 | +194.40% |
| Load/for_loop_vars | 98 | -18.33% | 77 | -13.48% | 0 |  | 1,236 | +0.73% |
| Render/for_loop_vars | 96 | -87.64% | 9 | +12.50% | 0 |  | 4,055 | +3.84% |
| Load/for_range | 33 | -10.81% | 27 | -3.57% | 0 |  | 821 | -2.84% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 187 | -16.89% |
| Load/html_autoescape | 1,044 | -4.22% | 787 | -1.38% | 0 |  | 5,265 | +2.09% |
| Render/html_autoescape | 12,667 | +1.45% | 2,819 | -2.36% | 0 |  | 60,911 | +3.18% |
| Load/inheritance | 37 | -13.95% | 24 | -11.11% | 0 |  | 889 | -1.66% |
| Render/inheritance | 548 | -0.36% | 102 | 0% | 0 |  | 779 | -32.67% |
| Load/large_static | 2,759 | 0% | 1,209 | 0% | 0 |  | 510 | +18.60% |
| Render/large_static | 830 | -0.24% | 576 | 0% | 0 |  | 4 | -71.43% |
| Load/macros | 256 | -0.78% | 244 | +0.41% | 0 |  | 1,852 | +3.12% |
| Render/macros | 675 | +1.05% | 223 | +1.36% | 0 |  | 9,435 | +27.04% |
| Load/many_tags | 49,588 | -5.13% | 23,116 | -4.59% | 0 |  | 311,105 | -3.09% |
| Render/many_tags | 11,798 | -0.28% | 1,862 | -0.32% | 0 |  | 5,409 | +165.67% |
| Load/mitsuhiko_table | 167 | -8.74% | 197 | -1.99% | 0 |  | 1,199 | +2.48% |
| Render/mitsuhiko_table | 8,066 | +0.05% | 5,123 | +0.02% | 0 |  | 24,505 | +157.24% |
| Load/mitsuhiko_table_wide | 303 | -4.11% | 335 | -0.59% | 0 |  | 1,684 | -6.18% |
| Render/mitsuhiko_table_wide | 8,130 | +0.06% | 21,517 | 0% | 0 |  | 33,544 | +226.72% |
| Load/plain_text | 0 |  | 0 |  | 0 |  | 7 | -12.50% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| Load/strings | 374 | -9.22% | 253 | -4.53% | 0 |  | 3,035 | -6.01% |
| Render/strings | 930 | -0.75% | 1,338 | +0.38% | 0 |  | 4,738 | +117.54% |
| Load/substitute | 4 | 0% | 4 | 0% | 0 |  | 265 | -2.93% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -33.33% |

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
