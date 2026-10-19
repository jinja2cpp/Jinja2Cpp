# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (119 so far, latest [2acdacb](https://github.com/jinja2cpp/Jinja2Cpp/commit/2acdacb5fb24fe7ef999ee0635334a53527cf1a8) on 2026-10-19). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 349,491 | -1.21% |  | 57 | 0% |  | 18,080 | 0% | -53.74% |
| [Render/chat_llama](#renderchat_llama) | 306,184 | -0.12% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 400,141 | -1.32% |  | 50 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 514,017 | -0.15% |  | 428 | 0% |  | 9,536 | +0.17% | -27.80% |
| [Load/chat_qwen](#loadchat_qwen) | 269,382 | -1.13% |  | 44 | 0% |  | 14,320 | 0% | -47.60% |
| [Render/chat_qwen](#renderchat_qwen) | 293,391 | -0.14% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 100,212 | -1.46% |  | 27 | 0% |  | 5,336 | 0% | -49.47% |
| [Render/config_file](#renderconfig_file) | 1,256,561 | -0.23% |  | 908 | 0% |  | 23,304 | -1.89% | -2.12% |
| [Load/dict_ops](#loaddict_ops) | 33,416 | -2.34% | -77.39% | 9 | 0% | -93.13% | 2,160 | 0% | -49.25% |
| [Render/dict_ops](#renderdict_ops) | 299,284 | -0.06% | -56.68% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,975 | -0.93% | -77.70% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 454,302 | 0% | -49.86% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,956 | -1.61% | -73.42% | 29 | 0% | -87.28% | 3,672 | 0% | -67.83% |
| [Render/filters](#renderfilters) | 31,161 | -0.01% | -64.86% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,326 | -1.95% | -77.60% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,378 | +0.02% | -68.64% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,781 | -1.73% | -77.43% | 10 | 0% | -93.63% | 2,368 | +0.68% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,527 | -0.06% | -81.87% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,739 | -2.20% | -75.76% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 39,235 | -0.06% | -57.45% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 197,335 | -2.13% |  | 50 | 0% |  | 12,320 | 0% | -50.08% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,232,778 | -0.11% |  | 734 | 0% |  | 43,664 | -0.26% | +8.12% |
| [Load/inheritance](#loadinheritance) | 23,296 | -2.19% | -79.59% | 7 | 0% | -93.46% | 1,424 | 0% | -50.42% |
| [Render/inheritance](#renderinheritance) | 136,397 | -0.28% | -81.76% | 35 | 0% | -94.37% | 6,456 | -3.35% | +7.31% |
| [Load/large_static](#loadlarge_static) | 154,561 | -1.64% | -82.55% | 18 | 0% | -95.91% | 42,736 | 0% | -19.95% |
| [Render/large_static](#renderlarge_static) | 21,062 | -0.11% | -71.97% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 56,103 | -1.66% | -75.39% | 28 | 0% | -84.95% | 4,312 | 0% | -30.63% |
| [Render/macros](#rendermacros) | 648,939 | +0.04% | -74.68% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,144,141 | -1.95% | -79.30% | 956 | 0% | -97.21% | 421,408 | -0.01% | -60.06% |
| [Render/many_tags](#rendermany_tags) | 732,776 | -0.28% | -64.82% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,969 | -2.53% | -80.23% | 20 | 0% | -89.13% | 3,328 | 0% | -45.12% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,197,643 | +0.01% | -57.94% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 56,255 | -1.48% |  | 36 | 0% |  | 5,656 | 0% | -31.56% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,542,448 | -0.01% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [Load/plain_text](#loadplain_text) | 3,756 | -2.29% | -61.18% | 4 | 0% | -77.78% | 704 | 0% | -37.14% |
| [Render/plain_text](#renderplain_text) | 1,036 | -2.15% | -84.75% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 73,307 | -1.57% | -74.88% | 30 | 0% | -86.61% | 4,232 | 0% | -53.19% |
| [Render/strings](#renderstrings) | 633,603 | +0.06% | -69.79% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,771 | -2.39% | -73.97% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,274 | -0.99% | -74.20% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 738 | +5.88% | 733 | +0.83% | 0 |  | 7,591 | -1.86% |
| Render/chat_llama | 1,128 | -3.42% | 463 | -4.93% | 0 |  | 3,730 | +8.02% |
| Load/chat_mistral | 1,065 | +6.50% | 1,021 | +2.72% | 0 |  | 8,847 | +2.36% |
| Render/chat_mistral | 1,734 | -5.81% | 485 | -13.08% | 0 |  | 8,798 | -1.84% |
| Load/chat_qwen | 497 | -4.42% | 604 | -1.31% | 0 |  | 4,746 | +0.70% |
| Render/chat_qwen | 993 | -6.76% | 398 | -2.45% | 0 |  | 3,230 | +4.94% |
| Load/config_file | 169 | -1.74% | 150 | -16.20% | 0 |  | 3,070 | +3.51% |
| Render/config_file | 5,808 | -1.63% | 1,916 | +4.30% | 0 |  | 37,927 | -1.31% |
| Load/dict_ops | 31 | -22.50% | 15 | -44.44% | 0 |  | 1,330 | -2.06% |
| Render/dict_ops | 723 | -5.86% | 419 | -1.87% | 0 |  | 863 | -0.46% |
| Load/expressions | 21 | -41.67% | 12 | -55.56% | 0 |  | 1,339 | -0.07% |
| Render/expressions | 3 | -62.50% | 1 | -50.00% | 0 |  | 274 | -11.33% |
| Load/filters | 123 | -2.38% | 64 | -18.99% | 0 |  | 1,967 | +5.24% |
| Render/filters | 17 | -48.48% | 4 | -42.86% | 0 |  | 761 | +1.74% |
| Load/for_filter_if | 8 | 0% | 3 | -62.50% | 0 |  | 1,165 | +5.05% |
| Render/for_filter_if | 1,655 | -0.42% | 61 | -18.67% | 0 |  | 178 | -0.56% |
| Load/for_loop_vars | 28 | +55.56% | 10 | -28.57% | 0 |  | 1,196 | +8.73% |
| Render/for_loop_vars | 4 | -66.67% | 5 | -28.57% | 0 |  | 148 | +2.07% |
| Load/for_range | 5 | +150.00% | 4 | -20.00% | 0 |  | 727 | -1.22% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 109 | -4.39% |
| Load/html_autoescape | 432 | -2.26% | 479 | -1.24% | 0 |  | 4,581 | +13.62% |
| Render/html_autoescape | 4,631 | +25.37% | 1,355 | +2.65% | 0 |  | 32,197 | +3.31% |
| Load/inheritance | 2 | 0% | 2 | -60.00% | 0 |  | 632 | -5.95% |
| Render/inheritance | 574 | -3.69% | 120 | -8.40% | 0 |  | 779 | -2.38% |
| Load/large_static | 1,944 | -0.56% | 886 | -1.01% | 0 |  | 201 | -33.88% |
| Render/large_static | 705 | -0.56% | 570 | -0.18% | 0 |  | 2 | -86.67% |
| Load/macros | 67 | -18.29% | 52 | -18.75% | 0 |  | 1,711 | -0.06% |
| Render/macros | 740 | +0.27% | 232 | -7.57% | 0 |  | 1,729 | -23.56% |
| Load/many_tags | 28,848 | +3.91% | 21,643 | +0.93% | 0 |  | 261,203 | +5.84% |
| Render/many_tags | 6,085 | +0.90% | 906 | +4.98% | 0 |  | 164 | +5.13% |
| Load/mitsuhiko_table | 26 | -7.14% | 19 | -56.82% | 0 |  | 858 | -5.40% |
| Render/mitsuhiko_table | 9,034 | -0.32% | 6,598 | -0.30% | 0 |  | 276 | -91.26% |
| Load/mitsuhiko_table_wide | 76 | +8.57% | 73 | -8.75% | 0 |  | 971 | +0.31% |
| Render/mitsuhiko_table_wide | 9,158 | +0.18% | 23,022 | +0.48% | 0 |  | 274 | -8.36% |
| Load/plain_text | 0 |  | 2 | -33.33% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 1 | 0% | 0 |  | 4 | -50.00% |
| Load/strings | 166 | +2.47% | 95 | -12.84% | 0 |  | 2,753 | -1.57% |
| Render/strings | 475 | +1.50% | 611 | -0.33% | 0 |  | 1,472 | -0.81% |
| Load/substitute | 1 | -50.00% | 2 | -50.00% | 0 |  | 106 | -32.48% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 2 | -84.62% |

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
