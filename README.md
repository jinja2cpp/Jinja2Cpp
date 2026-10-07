# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (47 so far, latest [8a2018d](https://github.com/jinja2cpp/Jinja2Cpp/commit/8a2018dd66649f464ec344c4117b66e9f8040ec9) on 2026-10-07). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 577,126 | +7.92% |  | 137 | +4.58% |  | 23,752 | -39.10% | -39.22% |
| [Render/chat_llama](#renderchat_llama) | 410,892 | +0.12% |  | 348 | 0% |  | 13,232 | +0.12% | -7.86% |
| [Load/chat_mistral](#loadchat_mistral) | 674,758 | +9.11% |  | 127 | +3.25% |  | 29,424 | -23.69% | -34.10% |
| [Render/chat_mistral](#renderchat_mistral) | 712,667 | +0.43% |  | 475 | 0% |  | 11,664 | +0.55% | -11.69% |
| [Load/chat_qwen](#loadchat_qwen) | 428,998 | +8.65% |  | 98 | +2.08% |  | 18,496 | -49.98% | -32.32% |
| [Render/chat_qwen](#renderchat_qwen) | 398,014 | +0.37% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 148,274 | +10.63% |  | 43 | +4.88% |  | 6,816 | -25.98% | -35.45% |
| [Render/config_file](#renderconfig_file) | 2,003,565 | +0.27% |  | 1,704 | 0% |  | 21,488 | +0.30% | -9.74% |
| [Load/dict_ops](#loaddict_ops) | 52,796 | +11.41% | -64.27% | 19 | 0% | -85.50% | 2,888 | -37.87% | -32.14% |
| [Render/dict_ops](#renderdict_ops) | 403,323 | +0.33% | -41.62% | 315 | 0% | -30.00% | 34,096 | +0.14% | -1.73% |
| [Load/expressions](#loadexpressions) | 70,527 | +11.86% | -65.79% | 24 | 0% | -83.56% | 4,392 | -51.07% | -28.42% |
| [Render/expressions](#renderexpressions) | 532,223 | +1.50% | -41.26% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 99,603 | +8.28% | -63.71% | 46 | 0% | -79.82% | 4,792 | -49.58% | -58.02% |
| [Render/filters](#renderfilters) | 59,963 | -0.47% | -32.37% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 43,290 | +13.29% | -68.03% | 15 | 0% | -87.80% | 2,304 | -46.86% | -33.18% |
| [Render/for_filter_if](#renderfor_filter_if) | 463,878 | +0.96% | -46.40% | 206 | 0% | -8.85% | 2,568 | +0.63% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 58,129 | +12.02% | -67.82% | 17 | 0% | -89.17% | 2,904 | -34.12% | -35.75% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 276,282 | +1.07% | -74.25% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 21,531 | +10.80% | -62.02% | 12 | 0% | -78.57% | 1,416 | -36.33% | -29.20% |
| [Render/for_range](#renderfor_range) | 46,797 | +0.90% | -49.25% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 291,230 | +11.71% |  | 76 | +2.70% |  | 16,072 | -55.33% | -34.88% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,684,854 | +0.24% |  | 1,012 | 0% |  | 38,712 | +0.04% | -4.14% |
| [Load/inheritance](#loadinheritance) | 37,903 | +7.13% | -66.79% | 17 | -5.56% | -84.11% | 1,960 | -57.24% | -31.75% |
| [Render/inheritance](#renderinheritance) | 215,412 | +0.58% | -71.20% | 78 | 0% | -87.46% | 3,328 | +1.96% | -44.68% |
| [Load/large_static](#loadlarge_static) | 237,838 | +8.46% | -73.16% | 29 | +3.57% | -93.41% | 45,312 | -15.07% | -15.12% |
| [Render/large_static](#renderlarge_static) | 27,387 | +0.65% | -63.55% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,392 | +9.20% | -64.29% | 34 | 0% | -81.72% | 4,512 | -51.84% | -27.41% |
| [Render/macros](#rendermacros) | 1,175,164 | +0.54% | -54.15% | 407 | 0% | -85.09% | 13,592 | +0.12% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,669,774 | +12.97% | -69.05% | 2,158 | +0.14% | -93.70% | 561,936 | -29.88% | -46.75% |
| [Render/many_tags](#rendermany_tags) | 1,268,763 | -0.07% | -39.09% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 83,439 | +8.78% | -64.88% | 32 | 0% | -82.61% | 4,328 | -52.71% | -28.63% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,653,420 | +0.90% | -46.15% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 102,611 | +6.41% |  | 67 | 0% |  | 6,512 | -42.55% | -21.20% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,024,550 | +0.85% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,579 | +16.43% | -52.68% | 6 | 0% | -66.67% | 752 | -34.72% | -32.86% |
| [Render/plain_text](#renderplain_text) | 1,034 | +1.17% | -84.79% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 105,263 | +8.34% | -63.93% | 43 | 0% | -80.80% | 5,512 | -43.25% | -39.03% |
| [Render/strings](#renderstrings) | 1,063,319 | -0.01% | -49.31% | 976 | 0% | -53.83% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,288 | +9.02% | -62.19% | 9 | -10.00% | -75.68% | 960 | -56.36% | -36.84% |
| [Render/substitute](#rendersubstitute) | 2,714 | +1.65% | -69.21% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,713 | +44.31% | 1,644 | +28.14% | 0 |  | 8,619 | +4.08% |
| Render/chat_llama | 1,315 | -18.83% | 468 | -9.65% | 0 |  | 4,963 | -6.64% |
| Load/chat_mistral | 2,360 | +65.85% | 2,297 | +38.04% | 0 |  | 9,981 | +8.74% |
| Render/chat_mistral | 2,521 | -20.25% | 694 | -6.09% | 0 |  | 14,524 | -1.28% |
| Load/chat_qwen | 1,019 | +16.06% | 1,320 | +14.58% | 0 |  | 5,318 | +13.85% |
| Render/chat_qwen | 1,243 | -25.92% | 323 | -12.23% | 0 |  | 4,365 | -1.93% |
| Load/config_file | 428 | +8.91% | 575 | +17.59% | 0 |  | 3,319 | +7.97% |
| Render/config_file | 8,891 | -14.10% | 2,487 | +0.12% | 0 |  | 73,836 | -2.58% |
| Load/dict_ops | 122 | +28.42% | 113 | +56.94% | 0 |  | 1,676 | +8.62% |
| Render/dict_ops | 1,077 | -4.61% | 610 | -2.40% | 0 |  | 1,121 | -0.97% |
| Load/expressions | 164 | +36.67% | 239 | +64.83% | 0 |  | 1,542 | +7.53% |
| Render/expressions | 4 | -71.43% | 2 | -60.00% | 0 |  | 455 | +10.98% |
| Load/filters | 246 | +7.89% | 171 | +23.02% | 0 |  | 2,072 | -4.95% |
| Render/filters | 140 | -16.67% | 79 | -14.13% | 0 |  | 1,196 | +2.31% |
| Load/for_filter_if | 53 | +65.62% | 38 | +72.73% | 0 |  | 1,171 | +10.68% |
| Render/for_filter_if | 1,246 | -1.19% | 50 | -1.96% | 0 |  | 1,209 | -7.92% |
| Load/for_loop_vars | 85 | +25.00% | 101 | +53.03% | 0 |  | 1,327 | +14.10% |
| Render/for_loop_vars | 8 | -66.67% | 2 | -77.78% | 0 |  | 317 | +9.31% |
| Load/for_range | 33 | +94.12% | 29 | +81.25% | 0 |  | 992 | +10.22% |
| Render/for_range | 3 | +50.00% | 2 | +100.00% | 0 |  | 290 | +32.42% |
| Load/html_autoescape | 860 | +20.96% | 955 | +14.65% | 0 |  | 4,339 | +12.99% |
| Render/html_autoescape | 5,642 | -32.60% | 1,647 | -13.45% | 0 |  | 54,870 | +3.78% |
| Load/inheritance | 29 | +20.83% | 26 | +23.81% | 0 |  | 1,043 | +9.56% |
| Render/inheritance | 543 | -6.86% | 97 | 0% | 0 |  | 1,434 | +81.75% |
| Load/large_static | 2,624 | +0.61% | 1,259 | +6.42% | 0 |  | 460 | +30.68% |
| Render/large_static | 754 | -8.38% | 575 | +0.17% | 0 |  | 12 | +200.00% |
| Load/macros | 193 | +20.63% | 239 | +35.80% | 0 |  | 1,886 | +10.10% |
| Render/macros | 717 | +4.22% | 227 | +1.34% | 0 |  | 12,435 | +29.84% |
| Load/many_tags | 47,204 | +68.08% | 34,934 | +44.28% | 0 |  | 276,731 | +0.01% |
| Render/many_tags | 8,697 | -25.21% | 1,816 | -2.31% | 0 |  | 5,378 | +4.77% |
| Load/mitsuhiko_table | 129 | +43.33% | 214 | +53.96% | 0 |  | 1,398 | +15.73% |
| Render/mitsuhiko_table | 8,042 | -0.15% | 5,118 | -0.06% | 0 |  | 988 | +116.67% |
| Load/mitsuhiko_table_wide | 224 | +2.28% | 370 | +25.00% | 0 |  | 1,864 | +11.15% |
| Render/mitsuhiko_table_wide | 8,108 | -1.36% | 21,519 | 0% | 0 |  | 491 | -93.19% |
| Load/plain_text | 0 |  | 1 | 0% | 0 |  | 26 | +225.00% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 6 | +50.00% |
| Load/strings | 309 | +16.60% | 304 | +31.60% | 0 |  | 2,979 | +3.83% |
| Render/strings | 933 | -5.85% | 1,318 | -1.93% | 0 |  | 2,056 | +1.43% |
| Load/substitute | 5 | +25.00% | 6 | 0% | 0 |  | 361 | +26.22% |
| Render/substitute | 0 |  | 1 |  | 0 |  | 5 | +25.00% |

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
