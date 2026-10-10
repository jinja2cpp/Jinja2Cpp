# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (64 so far, latest [08bce40](https://github.com/jinja2cpp/Jinja2Cpp/commit/08bce40d555dfe1a29ae5661b1cac1e6f873da7a) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 555,170 | 0% |  | 92 | 0% |  | 19,856 | 0% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 407,032 | +0.03% |  | 353 | +1.44% |  | 15,176 | +14.83% | +5.68% |
| [Load/chat_mistral](#loadchat_mistral) | 657,629 | 0% |  | 75 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 654,550 | +0.02% |  | 431 | +1.17% |  | 12,352 | +18.86% | -6.48% |
| [Load/chat_qwen](#loadchat_qwen) | 424,516 | 0% |  | 68 | 0% |  | 15,760 | 0% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 393,768 | +0.01% |  | 339 | +0.59% |  | 7,920 | +7.61% | -6.87% |
| [Load/config_file](#loadconfig_file) | 145,958 | 0% |  | 34 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,735,813 | +0.01% |  | 1,179 | +0.86% |  | 23,896 | +10.34% | +0.37% |
| [Load/dict_ops](#loaddict_ops) | 47,706 | 0% | -67.72% | 12 | 0% | -90.84% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 351,801 | +0.01% | -49.08% | 114 | +1.79% | -74.67% | 36,168 | +1.57% | +4.24% |
| [Load/expressions](#loadexpressions) | 67,192 | 0% | -67.41% | 18 | 0% | -87.67% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 517,674 | 0% | -42.87% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,655 | 0% | -64.42% | 32 | 0% | -85.96% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,803 | +0.09% | -40.45% | 37 | +5.71% | -50.00% | 2,552 | +28.11% | -48.05% |
| [Load/for_filter_if](#loadfor_filter_if) | 41,106 | 0% | -69.64% | 9 | 0% | -92.68% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 447,640 | +0.01% | -48.28% | 208 | +0.97% | -7.96% | 3,128 | +21.81% | -14.07% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 57,243 | 0% | -68.31% | 13 | 0% | -91.72% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,288 | +0.02% | -74.62% | 4 | +100.00% | -99.23% | 2,112 | +36.08% | -23.70% |
| [Load/for_range](#loadfor_range) | 19,594 | 0% | -65.43% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,430 | 0% | -49.65% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 289,624 | 0% |  | 61 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,599,485 | +0.01% |  | 1,013 | +1.00% |  | 41,600 | +7.17% | +3.01% |
| [Load/inheritance](#loadinheritance) | 34,687 | 0% | -69.60% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 193,711 | +0.09% | -74.10% | 81 | +10.96% | -86.98% | 4,416 | +33.98% | -26.60% |
| [Load/large_static](#loadlarge_static) | 237,668 | 0% | -73.17% | 28 | 0% | -93.64% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,486 | +0.22% | -71.40% | 3 | +200.00% | -83.33% | 37,112 | +1.53% | -0.32% |
| [Load/macros](#loadmacros) | 81,731 | +0.08% | -64.15% | 32 | +10.34% | -82.80% | 5,128 | +37.55% | -17.50% |
| [Render/macros](#rendermacros) | 1,007,580 | +0.01% | -60.69% | 410 | +0.99% | -84.98% | 14,728 | +8.23% | +0.05% |
| [Load/many_tags](#loadmany_tags) | 13,310,876 | 0% | -69.86% | 962 | 0% | -97.19% | 445,680 | 0% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,165,906 | +0.03% | -44.03% | 15 | +650.00% | -28.57% | 82,520 | +1365.20% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 78,111 | 0% | -67.12% | 26 | 0% | -85.87% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,623,833 | 0% | -46.39% | 1,027 | +0.20% | -74.66% | 346,424 | +0.16% | -0.17% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 96,464 | 0% |  | 57 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,994,221 | 0% |  | 1,045 | +0.19% |  | 1,380,352 | +0.04% | -0.05% |
| [Load/plain_text](#loadplain_text) | 4,068 | 0% | -57.96% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 101,400 | 0% | -65.26% | 33 | 0% | -85.27% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,055,236 | +0.01% | -49.69% | 979 | +0.41% | -53.69% | 18,368 | +6.49% | +2.45% |
| [Load/substitute](#loadsubstitute) | 10,865 | 0% | -63.61% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,711 | +1.84% | -69.25% | 3 | +200.00% | -75.00% | 600 | +1400.00% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,188 | 0% | 1,385 | 0% | 0 |  | 8,376 | -7.02% |
| Render/chat_llama | 1,275 | 0% | 508 | 0% | 0 |  | 4,722 | +2.79% |
| Load/chat_mistral | 1,814 | 0% | 2,066 | 0% | 0 |  | 9,818 | -6.36% |
| Render/chat_mistral | 2,176 | 0% | 538 | 0% | 0 |  | 13,503 | +1.21% |
| Load/chat_qwen | 836 | 0% | 1,240 | 0% | 0 |  | 5,496 | -7.88% |
| Render/chat_qwen | 1,209 | 0% | 380 | 0% | 0 |  | 4,176 | -4.07% |
| Load/config_file | 380 | 0% | 562 | 0% | 0 |  | 3,485 | -3.25% |
| Render/config_file | 7,052 | 0% | 2,046 | 0% | 0 |  | 69,220 | +0.54% |
| Load/dict_ops | 106 | 0% | 109 | 0% | 0 |  | 1,568 | -0.95% |
| Render/dict_ops | 949 | 0% | 728 | 0% | 0 |  | 1,101 | -0.63% |
| Load/expressions | 130 | 0% | 204 | 0% | 0 |  | 1,507 | -1.63% |
| Render/expressions | 10 | 0% | 3 | 0% | 0 |  | 448 | -6.47% |
| Load/filters | 208 | 0% | 151 | 0% | 0 |  | 2,484 | -5.59% |
| Render/filters | 56 | 0% | 30 | 0% | 0 |  | 1,140 | +0.44% |
| Load/for_filter_if | 53 | 0% | 42 | 0% | 0 |  | 1,179 | -3.60% |
| Render/for_filter_if | 1,310 | 0% | 58 | 0% | 0 |  | 382 | -5.68% |
| Load/for_loop_vars | 55 | 0% | 75 | 0% | 0 |  | 1,331 | -3.20% |
| Render/for_loop_vars | 9 | 0% | 6 | 0% | 0 |  | 283 | +4.81% |
| Load/for_range | 21 | 0% | 26 | 0% | 0 |  | 946 | -1.56% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 239 | +2.58% |
| Load/html_autoescape | 654 | 0% | 880 | 0% | 0 |  | 4,955 | -1.78% |
| Render/html_autoescape | 4,170 | 0% | 1,485 | 0% | 0 |  | 51,108 | +3.23% |
| Load/inheritance | 25 | 0% | 23 | 0% | 0 |  | 911 | -4.21% |
| Render/inheritance | 505 | 0% | 101 | 0% | 0 |  | 1,198 | -40.22% |
| Load/large_static | 2,619 | 0% | 1,190 | 0% | 0 |  | 529 | +2.32% |
| Render/large_static | 712 | 0% | 579 | 0% | 0 |  | 4 | -63.64% |
| Load/macros | 214 | 0% | 274 | 0% | 0 |  | 1,983 | -3.46% |
| Render/macros | 666 | 0% | 208 | 0% | 0 |  | 9,557 | -3.31% |
| Load/many_tags | 40,152 | 0% | 33,258 | 0% | 0 |  | 305,610 | -3.34% |
| Render/many_tags | 6,881 | +0.06% | 1,463 | 0% | 0 |  | 525 | -7.08% |
| Load/mitsuhiko_table | 121 | 0% | 205 | 0% | 0 |  | 1,260 | -2.63% |
| Render/mitsuhiko_table | 8,021 | 0% | 5,014 | 0% | 0 |  | 471 | -1.88% |
| Load/mitsuhiko_table_wide | 198 | 0% | 335 | 0% | 0 |  | 1,536 | -2.78% |
| Render/mitsuhiko_table_wide | 8,077 | 0% | 21,460 | 0% | 0 |  | 497 | +1.43% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 38 | +90.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 256 | 0% | 282 | 0% | 0 |  | 3,158 | -2.41% |
| Render/strings | 913 | 0% | 1,349 | 0% | 0 |  | 4,081 | +53.54% |
| Load/substitute | 7 | 0% | 12 | 0% | 0 |  | 366 | +4.57% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 5 | +66.67% |

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
