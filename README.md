# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (50 so far, latest [3cc8e7d](https://github.com/jinja2cpp/Jinja2Cpp/commit/3cc8e7df4bba98cb4c6291c978e8efcb1b28bd42) on 2026-10-08). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 582,568 | +2.08% |  | 121 | +0.83% |  | 23,408 | 0% | -40.10% |
| [Render/chat_llama](#renderchat_llama) | 410,569 | 0% |  | 348 | 0% |  | 13,232 | 0% | -7.86% |
| [Load/chat_mistral](#loadchat_mistral) | 683,925 | +2.18% |  | 113 | +0.89% |  | 29,112 | 0% | -34.80% |
| [Render/chat_mistral](#renderchat_mistral) | 711,760 | -0.03% |  | 474 | 0% |  | 11,648 | 0% | -11.81% |
| [Load/chat_qwen](#loadchat_qwen) | 437,823 | +2.05% |  | 96 | +1.05% |  | 18,440 | 0% | -32.52% |
| [Render/chat_qwen](#renderchat_qwen) | 397,655 | 0% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 151,837 | +2.51% |  | 41 | +2.50% |  | 6,728 | 0% | -36.29% |
| [Render/config_file](#renderconfig_file) | 2,000,711 | +0.04% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 54,169 | +2.41% | -63.34% | 18 | 0% | -86.26% | 2,864 | 0% | -32.71% |
| [Render/dict_ops](#renderdict_ops) | 401,080 | 0% | -41.94% | 315 | 0% | -30.00% | 34,016 | 0% | -1.96% |
| [Load/expressions](#loadexpressions) | 72,225 | +2.54% | -64.96% | 23 | 0% | -84.25% | 4,384 | 0% | -28.55% |
| [Render/expressions](#renderexpressions) | 532,368 | 0% | -41.25% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 100,595 | +3.63% | -63.35% | 33 | 0% | -85.53% | 4,488 | 0% | -60.69% |
| [Render/filters](#renderfilters) | 60,076 | 0% | -32.25% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 44,417 | +2.64% | -67.19% | 15 | 0% | -87.80% | 2,304 | 0% | -33.18% |
| [Render/for_filter_if](#renderfor_filter_if) | 463,772 | 0% | -46.42% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 59,853 | +2.58% | -66.87% | 17 | 0% | -89.17% | 2,904 | 0% | -35.75% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 276,232 | 0% | -74.25% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 21,787 | +2.48% | -61.56% | 12 | 0% | -78.57% | 1,416 | 0% | -29.20% |
| [Render/for_range](#renderfor_range) | 46,749 | 0% | -49.30% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 299,772 | +2.85% |  | 72 | +1.41% |  | 15,968 | 0% | -35.30% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,684,199 | +0.02% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 38,704 | +2.23% | -66.08% | 17 | 0% | -84.11% | 1,960 | 0% | -31.75% |
| [Render/inheritance](#renderinheritance) | 213,313 | +0.10% | -71.48% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 244,343 | +2.55% | -72.42% | 30 | +3.45% | -93.18% | 45,312 | 0% | -15.12% |
| [Render/large_static](#renderlarge_static) | 27,370 | 0% | -63.57% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 83,097 | +2.21% | -63.55% | 34 | 0% | -81.72% | 4,512 | 0% | -27.41% |
| [Render/macros](#rendermacros) | 1,175,552 | 0% | -54.14% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,899,297 | +2.87% | -68.53% | 1,561 | +0.06% | -95.44% | 547,696 | 0% | -48.10% |
| [Render/many_tags](#rendermany_tags) | 1,269,151 | 0% | -39.07% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 85,212 | +2.30% | -64.13% | 32 | 0% | -82.61% | 4,328 | 0% | -28.63% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,653,397 | 0% | -46.15% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 104,545 | +1.81% |  | 67 | 0% |  | 6,512 | 0% | -21.20% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,024,443 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,780 | +4.55% | -50.60% | 6 | 0% | -66.67% | 752 | 0% | -32.86% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 108,325 | +2.60% | -62.88% | 37 | 0% | -83.48% | 5,360 | 0% | -40.71% |
| [Render/strings](#renderstrings) | 1,062,607 | 0% | -49.34% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,656 | +3.15% | -60.96% | 9 | 0% | -75.68% | 960 | 0% | -36.84% |
| [Render/substitute](#rendersubstitute) | 2,666 | 0% | -69.76% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,762 | +5.45% | 1,664 | +1.46% | 0 |  | 9,020 | +4.24% |
| Render/chat_llama | 1,391 | +7.50% | 458 | +0.66% | 0 |  | 5,104 | +2.33% |
| Load/chat_mistral | 2,482 | +3.03% | 2,276 | -1.00% | 0 |  | 10,294 | +6.31% |
| Render/chat_mistral | 2,551 | +4.34% | 712 | +2.89% | 0 |  | 14,099 | -3.66% |
| Load/chat_qwen | 1,178 | +11.45% | 1,407 | +5.24% | 0 |  | 5,671 | +2.59% |
| Render/chat_qwen | 1,250 | 0% | 317 | -7.31% | 0 |  | 4,350 | -1.45% |
| Load/config_file | 420 | +3.19% | 585 | -1.35% | 0 |  | 3,645 | +7.08% |
| Render/config_file | 8,205 | +3.53% | 2,266 | +1.43% | 0 |  | 74,620 | -0.92% |
| Load/dict_ops | 104 | -3.70% | 104 | -2.80% | 0 |  | 1,814 | +8.95% |
| Render/dict_ops | 1,037 | +1.27% | 624 | -0.48% | 0 |  | 1,117 | -1.24% |
| Load/expressions | 162 | +4.52% | 241 | -1.23% | 0 |  | 1,629 | +7.88% |
| Render/expressions | 14 | +55.56% | 5 | +25.00% | 0 |  | 447 | +1.82% |
| Load/filters | 222 | -0.89% | 174 | -4.40% | 0 |  | 2,617 | +21.72% |
| Render/filters | 103 | +4.04% | 77 | +6.94% | 0 |  | 1,157 | -2.28% |
| Load/for_filter_if | 46 | -14.81% | 30 | 0% | 0 |  | 1,289 | +9.80% |
| Render/for_filter_if | 1,307 | +0.08% | 52 | 0% | 0 |  | 429 | -7.34% |
| Load/for_loop_vars | 82 | +15.49% | 98 | +7.69% | 0 |  | 1,527 | +12.44% |
| Render/for_loop_vars | 21 | +162.50% | 4 | +33.33% | 0 |  | 296 | -0.34% |
| Load/for_range | 39 | +2.63% | 34 | +13.33% | 0 |  | 1,051 | +6.48% |
| Render/for_range | 3 | 0% | 2 | 0% | 0 |  | 227 | +2.25% |
| Load/html_autoescape | 968 | +3.53% | 1,010 | +2.96% | 0 |  | 5,306 | +14.97% |
| Render/html_autoescape | 6,218 | +0.97% | 1,954 | +1.14% | 0 |  | 55,531 | +2.47% |
| Load/inheritance | 26 | +23.81% | 22 | -15.38% | 0 |  | 1,123 | +8.61% |
| Render/inheritance | 522 | +1.75% | 100 | -0.99% | 0 |  | 840 | -34.63% |
| Load/large_static | 2,654 | +0.26% | 1,257 | +0.40% | 0 |  | 1,394 | +228.77% |
| Render/large_static | 753 | +0.13% | 582 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 225 | -2.17% | 281 | +1.08% | 0 |  | 2,069 | +9.18% |
| Render/macros | 728 | +8.01% | 214 | -2.28% | 0 |  | 12,225 | +30.51% |
| Load/many_tags | 50,041 | +0.34% | 34,390 | -1.98% | 0 |  | 306,917 | +9.87% |
| Render/many_tags | 8,321 | +0.53% | 1,771 | -0.11% | 0 |  | 5,206 | -25.20% |
| Load/mitsuhiko_table | 121 | +1.68% | 239 | +4.82% | 0 |  | 1,510 | +15.53% |
| Render/mitsuhiko_table | 8,038 | +0.11% | 5,009 | 0% | 0 |  | 460 | +0.66% |
| Load/mitsuhiko_table_wide | 239 | +10.14% | 341 | +1.19% | 0 |  | 1,878 | +3.24% |
| Render/mitsuhiko_table_wide | 8,304 | +2.70% | 21,461 | +0.02% | 0 |  | 501 | +1.83% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 14 | +75.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| Load/strings | 288 | -0.69% | 313 | +3.99% | 0 |  | 3,306 | +6.92% |
| Render/strings | 944 | -0.32% | 1,342 | +0.52% | 0 |  | 3,196 | +60.60% |
| Load/substitute | 4 | 0% | 6 | +20.00% | 0 |  | 422 | +18.21% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | +66.67% |

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
