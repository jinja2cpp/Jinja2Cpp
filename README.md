# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (73 so far, latest [ce547f1](https://github.com/jinja2cpp/Jinja2Cpp/commit/ce547f1ebe74cd67f2ed37c86699796619125871) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 363,987 | -2.20% |  | 76 | +28.81% |  | 18,680 | -6.22% | -52.20% |
| [Render/chat_llama](#renderchat_llama) | 372,959 | -8.18% |  | 421 | +20.29% |  | 12,632 | -5.39% | -12.03% |
| [Load/chat_mistral](#loadchat_mistral) | 427,321 | -5.31% |  | 63 | +10.53% |  | 21,664 | -9.10% | -51.48% |
| [Render/chat_mistral](#renderchat_mistral) | 560,511 | -12.22% |  | 491 | +18.03% |  | 8,832 | -13.62% | -33.13% |
| [Load/chat_qwen](#loadchat_qwen) | 282,178 | -3.53% |  | 57 | +26.67% |  | 14,800 | -6.75% | -45.84% |
| [Render/chat_qwen](#renderchat_qwen) | 355,232 | -8.74% |  | 395 | +24.21% |  | 8,864 | +0.27% | +4.23% |
| [Load/config_file](#loadconfig_file) | 100,194 | -2.48% |  | 27 | 0% |  | 5,320 | -4.59% | -49.62% |
| [Render/config_file](#renderconfig_file) | 1,420,575 | -13.88% |  | 1,281 | +14.38% |  | 23,416 | -3.53% | -1.65% |
| [Load/dict_ops](#loaddict_ops) | 33,390 | -2.20% | -77.41% | 9 | 0% | -93.13% | 2,144 | -3.60% | -49.62% |
| [Render/dict_ops](#renderdict_ops) | 304,463 | -12.02% | -55.93% | 112 | 0% | -75.11% | 24,408 | -31.45% | -29.65% |
| [Load/expressions](#loadexpressions) | 44,394 | -6.24% | -78.47% | 16 | 0% | -89.04% | 2,712 | -23.82% | -55.80% |
| [Render/expressions](#renderexpressions) | 446,332 | -12.00% | -50.74% | 3 | 0% | -90.00% | 2,440 | -1.29% | -31.15% |
| [Load/filters](#loadfilters) | 72,235 | -3.33% | -73.68% | 29 | 0% | -87.28% | 3,656 | -6.16% | -67.97% |
| [Render/filters](#renderfilters) | 46,721 | -8.87% | -47.31% | 51 | +45.71% | -31.08% | 1,376 | -31.47% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,078 | -1.24% | -77.78% | 7 | 0% | -94.31% | 1,888 | -2.48% | -45.24% |
| [Render/for_filter_if](#renderfor_filter_if) | 277,940 | -16.56% | -67.89% | 4 | 0% | -98.23% | 13,328 | -0.12% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,903 | -2.34% | -77.36% | 10 | 0% | -93.63% | 2,336 | -3.31% | -48.32% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 193,967 | -26.62% | -81.92% | 2 | 0% | -99.62% | 1,552 | -1.02% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,722 | -4.06% | -75.79% | 7 | -12.50% | -87.50% | 1,088 | -4.23% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,238 | -16.91% | -58.53% | 3 | 0% | -88.89% | 408 | -7.27% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 199,077 | -2.91% |  | 52 | +4.00% |  | 12,368 | -4.45% | -49.89% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,397,547 | -7.99% |  | 1,094 | +17.38% |  | 42,984 | -1.41% | +6.44% |
| [Load/inheritance](#loadinheritance) | 23,503 | -2.62% | -79.40% | 7 | 0% | -93.46% | 1,408 | -5.38% | -50.97% |
| [Render/inheritance](#renderinheritance) | 136,264 | -8.51% | -81.78% | 32 | 0% | -94.86% | 6,440 | -2.66% | +7.05% |
| [Load/large_static](#loadlarge_static) | 154,211 | -1.74% | -82.59% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,101 | +0.15% | -71.92% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 55,779 | -3.97% | -75.53% | 28 | 0% | -84.95% | 4,280 | -16.54% | -31.15% |
| [Render/macros](#rendermacros) | 687,690 | -20.16% | -73.17% | 308 | 0% | -88.71% | 18,944 | -1.25% | +28.70% |
| [Load/many_tags](#loadmany_tags) | 9,150,039 | -3.46% | -79.28% | 945 | 0% | -97.24% | 421,280 | -5.47% | -60.08% |
| [Render/many_tags](#rendermany_tags) | 915,215 | -12.35% | -56.06% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 47,642 | -4.04% | -79.95% | 20 | 0% | -89.13% | 3,312 | -6.76% | -45.38% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,114,111 | -20.75% | -58.61% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 68,479 | -0.13% |  | 51 | +13.33% |  | 5,832 | +1.67% | -29.43% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,460,005 | -19.75% |  | 33 | -35.29% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,667 | +0.33% | -62.10% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,000 | -0.20% | -85.29% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 72,919 | -3.62% | -75.01% | 30 | 0% | -86.61% | 4,216 | -6.39% | -53.36% |
| [Render/strings](#renderstrings) | 923,067 | -10.34% | -55.99% | 1,285 | +31.79% | -39.21% | 9,672 | -43.98% | -46.05% |
| [Load/substitute](#loadsubstitute) | 7,684 | -1.23% | -74.26% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,209 | -5.88% | -74.94% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 755 | -14.11% | 701 | -18.30% | 0 |  | 7,369 | +1.18% |
| Render/chat_llama | 1,120 | -12.50% | 371 | -24.13% | 0 |  | 3,842 | -16.84% |
| Load/chat_mistral | 1,112 | -14.66% | 1,024 | -14.45% | 0 |  | 8,139 | -4.10% |
| Render/chat_mistral | 1,828 | -17.14% | 425 | -18.58% | 0 |  | 9,063 | -30.55% |
| Load/chat_qwen | 585 | +1.21% | 600 | -11.76% | 0 |  | 4,397 | -3.89% |
| Render/chat_qwen | 1,065 | -11.98% | 331 | -14.47% | 0 |  | 3,142 | -21.00% |
| Load/config_file | 162 | -5.81% | 132 | -13.73% | 0 |  | 2,704 | -6.47% |
| Render/config_file | 5,580 | -19.04% | 1,571 | -18.60% | 0 |  | 41,113 | -27.41% |
| Load/dict_ops | 17 | -5.56% | 14 | -12.50% | 0 |  | 1,217 | -4.17% |
| Render/dict_ops | 598 | -38.73% | 398 | -45.55% | 0 |  | 833 | -17.03% |
| Load/expressions | 18 | -48.57% | 18 | -35.71% | 0 |  | 1,139 | -6.10% |
| Render/expressions | 3 | -25.00% | 1 | -66.67% | 0 |  | 254 | -39.23% |
| Load/filters | 116 | -1.69% | 89 | +3.49% | 0 |  | 1,958 | -1.01% |
| Render/filters | 13 | -84.15% | 2 | -94.74% | 0 |  | 927 | -13.12% |
| Load/for_filter_if | 13 | +18.18% | 6 | 0% | 0 |  | 947 | -7.79% |
| Render/for_filter_if | 1,579 | -1.25% | 57 | -13.64% | 0 |  | 149 | -45.22% |
| Load/for_loop_vars | 14 | -12.50% | 7 | -46.15% | 0 |  | 1,023 | -4.57% |
| Render/for_loop_vars | 4 | -20.00% | 1 | -50.00% | 0 |  | 118 | -47.09% |
| Load/for_range | 4 | -33.33% | 4 | -20.00% | 0 |  | 706 | -3.29% |
| Render/for_range | 1 | -50.00% | 1 | 0% | 0 |  | 72 | -59.32% |
| Load/html_autoescape | 443 | -2.85% | 490 | -7.89% | 0 |  | 3,869 | -2.52% |
| Render/html_autoescape | 2,825 | -49.13% | 1,124 | -28.22% | 0 |  | 39,594 | -13.43% |
| Load/inheritance | 1 | -87.50% | 3 | -40.00% | 0 |  | 593 | -0.34% |
| Render/inheritance | 555 | -7.04% | 114 | -8.80% | 0 |  | 727 | -13.66% |
| Load/large_static | 1,955 | -0.26% | 881 | -0.68% | 0 |  | 257 | +1.98% |
| Render/large_static | 706 | 0% | 575 | -0.17% | 0 |  | 3 | -50.00% |
| Load/macros | 75 | -25.74% | 66 | -22.35% | 0 |  | 1,602 | -2.97% |
| Render/macros | 751 | -3.47% | 234 | +7.83% | 0 |  | 1,510 | -23.08% |
| Load/many_tags | 26,562 | -6.65% | 20,594 | -5.92% | 0 |  | 236,348 | -0.89% |
| Render/many_tags | 6,021 | -7.23% | 688 | -20.65% | 0 |  | 3,431 | +1438.57% |
| Load/mitsuhiko_table | 28 | -30.00% | 25 | -35.90% | 0 |  | 743 | -5.59% |
| Render/mitsuhiko_table | 9,266 | -0.22% | 6,492 | -0.11% | 0 |  | 266 | -44.93% |
| Load/mitsuhiko_table_wide | 107 | +20.22% | 125 | +8.70% | 0 |  | 1,182 | -10.11% |
| Render/mitsuhiko_table_wide | 9,384 | -0.24% | 22,967 | -0.19% | 0 |  | 314 | -34.72% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 15 | +150.00% |
| Render/plain_text | 0 |  | 1 |  | 0 |  | 5 | -16.67% |
| Load/strings | 137 | -12.18% | 103 | -15.57% | 0 |  | 2,651 | -3.88% |
| Render/strings | 821 | -10.57% | 1,224 | -10.33% | 0 |  | 1,538 | -17.67% |
| Load/substitute | 1 | -50.00% | 3 | 0% | 0 |  | 138 | +16.95% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |

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
