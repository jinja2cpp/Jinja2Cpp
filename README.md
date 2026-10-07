# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (49 so far, latest [16f3f3b](https://github.com/jinja2cpp/Jinja2Cpp/commit/16f3f3bccfb40267498923801ba933a0c0711f00) on 2026-10-07). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 570,677 | -1.34% |  | 120 | -12.41% |  | 23,408 | -1.45% | -40.10% |
| [Render/chat_llama](#renderchat_llama) | 410,569 | -0.06% |  | 348 | 0% |  | 13,232 | 0% | -7.86% |
| [Load/chat_mistral](#loadchat_mistral) | 669,338 | -1.00% |  | 112 | -11.81% |  | 29,112 | -1.06% | -34.80% |
| [Render/chat_mistral](#renderchat_mistral) | 711,948 | -0.10% |  | 474 | -0.21% |  | 11,648 | -0.14% | -11.81% |
| [Load/chat_qwen](#loadchat_qwen) | 429,030 | -0.17% |  | 95 | -3.06% |  | 18,440 | -0.30% | -32.52% |
| [Render/chat_qwen](#renderchat_qwen) | 397,655 | -0.07% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 148,117 | -0.29% |  | 40 | -6.98% |  | 6,728 | -1.29% | -36.29% |
| [Render/config_file](#renderconfig_file) | 1,999,882 | +0.01% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 52,896 | +0.05% | -64.21% | 18 | -5.26% | -86.26% | 2,864 | -0.83% | -32.71% |
| [Render/dict_ops](#renderdict_ops) | 401,080 | -0.55% | -41.94% | 315 | 0% | -30.00% | 34,016 | -0.23% | -1.96% |
| [Load/expressions](#loadexpressions) | 70,438 | -0.26% | -65.83% | 23 | -4.17% | -84.25% | 4,384 | -0.18% | -28.55% |
| [Render/expressions](#renderexpressions) | 532,368 | +0.04% | -41.25% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,075 | -2.75% | -64.63% | 33 | -28.26% | -85.53% | 4,488 | -6.34% | -60.69% |
| [Render/filters](#renderfilters) | 60,076 | +0.27% | -32.25% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 43,273 | -0.14% | -68.04% | 15 | 0% | -87.80% | 2,304 | 0% | -33.18% |
| [Render/for_filter_if](#renderfor_filter_if) | 463,772 | -0.01% | -46.42% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 58,346 | +0.24% | -67.70% | 17 | 0% | -89.17% | 2,904 | 0% | -35.75% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 276,232 | 0% | -74.25% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 21,260 | -1.38% | -62.49% | 12 | 0% | -78.57% | 1,416 | 0% | -29.20% |
| [Render/for_range](#renderfor_range) | 46,749 | 0% | -49.30% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 291,469 | -0.11% |  | 71 | -6.58% |  | 15,968 | -0.65% | -35.30% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,683,779 | -0.01% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 37,860 | -0.25% | -66.82% | 17 | 0% | -84.11% | 1,960 | 0% | -31.75% |
| [Render/inheritance](#renderinheritance) | 213,097 | -0.15% | -71.51% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 238,261 | -0.12% | -73.11% | 29 | 0% | -93.41% | 45,312 | 0% | -15.12% |
| [Render/large_static](#renderlarge_static) | 27,370 | +0.11% | -63.57% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,299 | -0.28% | -64.33% | 34 | 0% | -81.72% | 4,512 | 0% | -27.41% |
| [Render/macros](#rendermacros) | 1,175,552 | +0.04% | -54.14% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,511,025 | -1.41% | -69.41% | 1,560 | -27.71% | -95.45% | 547,696 | -2.53% | -48.10% |
| [Render/many_tags](#rendermany_tags) | 1,269,151 | +0.03% | -39.07% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 83,298 | -0.26% | -64.94% | 32 | 0% | -82.61% | 4,328 | 0% | -28.63% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,653,397 | 0% | -46.15% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 102,689 | -0.20% |  | 67 | 0% |  | 6,512 | 0% | -21.20% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,024,443 | 0% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,572 | 0% | -52.75% | 6 | 0% | -66.67% | 752 | 0% | -32.86% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 105,581 | +0.08% | -63.82% | 37 | -13.95% | -83.48% | 5,360 | -2.76% | -40.71% |
| [Render/strings](#renderstrings) | 1,062,582 | -0.06% | -49.34% | 975 | -0.10% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,300 | -0.11% | -62.15% | 9 | 0% | -75.68% | 960 | 0% | -36.84% |
| [Render/substitute](#rendersubstitute) | 2,666 | 0% | -69.76% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,671 | -0.71% | 1,640 | +1.55% | 0 |  | 8,653 | -4.70% |
| Render/chat_llama | 1,294 | -0.31% | 455 | -4.21% | 0 |  | 4,988 | +2.07% |
| Load/chat_mistral | 2,409 | +3.84% | 2,299 | +1.46% | 0 |  | 9,683 | -6.62% |
| Render/chat_mistral | 2,445 | -5.42% | 692 | -0.43% | 0 |  | 14,634 | +0.64% |
| Load/chat_qwen | 1,057 | +2.52% | 1,337 | +1.44% | 0 |  | 5,528 | +1.41% |
| Render/chat_qwen | 1,250 | +1.05% | 342 | +5.23% | 0 |  | 4,414 | 0% |
| Load/config_file | 407 | -1.93% | 593 | +2.42% | 0 |  | 3,404 | +1.82% |
| Render/config_file | 7,925 | -18.17% | 2,234 | -15.44% | 0 |  | 75,311 | +3.14% |
| Load/dict_ops | 108 | -6.09% | 107 | -6.14% | 0 |  | 1,665 | -0.95% |
| Render/dict_ops | 1,024 | -4.57% | 627 | +3.29% | 0 |  | 1,131 | -2.08% |
| Load/expressions | 155 | 0% | 244 | +7.49% | 0 |  | 1,510 | -3.58% |
| Render/expressions | 9 | +125.00% | 4 | +100.00% | 0 |  | 439 | -4.15% |
| Load/filters | 224 | -2.18% | 182 | +6.43% | 0 |  | 2,150 | -5.99% |
| Render/filters | 99 | -30.28% | 72 | +1.41% | 0 |  | 1,184 | +1.72% |
| Load/for_filter_if | 54 | -14.29% | 30 | -34.78% | 0 |  | 1,174 | +0.51% |
| Render/for_filter_if | 1,306 | +5.32% | 52 | -3.70% | 0 |  | 463 | +4.04% |
| Load/for_loop_vars | 71 | -21.98% | 91 | -9.90% | 0 |  | 1,358 | +1.95% |
| Render/for_loop_vars | 8 | -20.00% | 3 | -40.00% | 0 |  | 297 | +14.67% |
| Load/for_range | 38 | +8.57% | 30 | 0% | 0 |  | 987 | -2.18% |
| Render/for_range | 3 | -62.50% | 2 | -33.33% | 0 |  | 222 | -9.76% |
| Load/html_autoescape | 935 | +9.48% | 981 | +3.26% | 0 |  | 4,615 | +2.53% |
| Render/html_autoescape | 6,158 | +19.60% | 1,932 | +59.41% | 0 |  | 54,190 | +2.15% |
| Load/inheritance | 21 | -8.70% | 26 | +13.04% | 0 |  | 1,034 | -3.54% |
| Render/inheritance | 513 | -7.90% | 101 | -2.88% | 0 |  | 1,285 | +57.48% |
| Load/large_static | 2,647 | +0.84% | 1,252 | -0.56% | 0 |  | 424 | -12.40% |
| Render/large_static | 752 | +0.40% | 582 | +0.52% | 0 |  | 3 | -70.00% |
| Load/macros | 230 | +10.05% | 278 | +14.88% | 0 |  | 1,895 | +0.58% |
| Render/macros | 674 | -33.00% | 219 | -35.96% | 0 |  | 9,367 | +11.83% |
| Load/many_tags | 49,873 | +4.29% | 35,084 | -1.40% | 0 |  | 279,337 | -5.33% |
| Render/many_tags | 8,277 | -4.86% | 1,773 | -1.99% | 0 |  | 6,960 | +230.33% |
| Load/mitsuhiko_table | 119 | +2.59% | 228 | +14.00% | 0 |  | 1,307 | -7.37% |
| Render/mitsuhiko_table | 8,029 | -0.15% | 5,009 | -2.21% | 0 |  | 457 | -2.14% |
| Load/mitsuhiko_table_wide | 217 | -3.56% | 337 | -8.42% | 0 |  | 1,819 | -7.62% |
| Render/mitsuhiko_table_wide | 8,086 | -0.41% | 21,457 | -0.30% | 0 |  | 492 | -4.65% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 8 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| Load/strings | 290 | -1.02% | 301 | +0.67% | 0 |  | 3,092 | +0.26% |
| Render/strings | 947 | +0.11% | 1,335 | +0.91% | 0 |  | 1,990 | -18.51% |
| Load/substitute | 4 | 0% | 5 | -16.67% | 0 |  | 357 | -6.30% |
| Render/substitute | 0 | -100.00% | 0 |  | 0 |  | 3 | -25.00% |

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
