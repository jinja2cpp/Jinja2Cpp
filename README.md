# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (57 so far, latest [50dec81](https://github.com/jinja2cpp/Jinja2Cpp/commit/50dec81d1855e11b7b69dd6caf871347a86e1980) on 2026-10-08). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 554,173 | -2.03% |  | 92 | 0% |  | 20,128 | -3.82% | -48.50% |
| [Render/chat_llama](#renderchat_llama) | 410,946 | -0.09% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 653,764 | -2.26% |  | 74 | 0% |  | 24,200 | -4.30% | -45.80% |
| [Render/chat_mistral](#renderchat_mistral) | 708,694 | -0.24% |  | 474 | 0% |  | 11,664 | +0.28% | -11.69% |
| [Load/chat_qwen](#loadchat_qwen) | 423,529 | -0.98% |  | 68 | 0% |  | 16,000 | -1.57% | -41.45% |
| [Render/chat_qwen](#renderchat_qwen) | 397,330 | -0.03% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 145,763 | -2.08% |  | 34 | 0% |  | 5,720 | -3.51% | -45.83% |
| [Render/config_file](#renderconfig_file) | 1,916,381 | +0.10% |  | 1,703 | 0% |  | 21,528 | 0% | -9.58% |
| [Load/dict_ops](#loaddict_ops) | 47,355 | -9.63% | -67.96% | 12 | -20.00% | -90.84% | 2,240 | -11.11% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 400,135 | -0.15% | -42.08% | 315 | 0% | -30.00% | 34,016 | -0.05% | -1.96% |
| [Load/expressions](#loadexpressions) | 66,930 | -1.87% | -67.53% | 18 | 0% | -87.67% | 3,592 | -1.75% | -41.46% |
| [Render/expressions](#renderexpressions) | 518,971 | 0% | -42.73% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 96,795 | -0.63% | -64.73% | 32 | 0% | -85.96% | 3,960 | 0% | -65.31% |
| [Render/filters](#renderfilters) | 60,297 | 0% | -32.00% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 40,925 | -2.26% | -69.77% | 9 | 0% | -92.68% | 1,952 | -3.94% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 447,512 | -2.98% | -48.29% | 206 | 0% | -8.85% | 2,568 | 0% | -29.45% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 56,956 | -1.61% | -68.47% | 13 | 0% | -91.72% | 2,448 | -2.55% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 275,055 | +0.01% | -74.36% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 19,493 | -5.37% | -65.61% | 10 | 0% | -82.14% | 1,136 | -5.33% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,776 | +0.09% | -49.27% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 287,383 | -1.21% |  | 63 | 0% |  | 13,416 | -1.53% | -45.64% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,613,474 | +0.09% |  | 1,011 | 0% |  | 38,752 | 0% | -4.04% |
| [Load/inheritance](#loadinheritance) | 34,956 | -2.86% | -69.37% | 10 | 0% | -90.65% | 1,552 | -3.96% | -45.96% |
| [Render/inheritance](#renderinheritance) | 214,360 | +0.17% | -71.34% | 76 | 0% | -87.78% | 3,328 | 0% | -44.68% |
| [Load/large_static](#loadlarge_static) | 236,792 | -0.52% | -73.27% | 28 | 0% | -93.64% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 27,520 | 0% | -63.37% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 81,769 | -1.00% | -64.13% | 31 | 0% | -83.33% | 3,832 | -2.04% | -38.35% |
| [Render/macros](#rendermacros) | 1,021,772 | +0.16% | -60.14% | 407 | 0% | -85.09% | 13,592 | 0% | -7.66% |
| [Load/many_tags](#loadmany_tags) | 13,199,788 | -2.15% | -70.11% | 958 | 0% | -97.20% | 457,856 | -3.83% | -56.61% |
| [Render/many_tags](#rendermany_tags) | 1,262,516 | -0.47% | -39.39% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 77,769 | -6.21% | -67.26% | 26 | -7.14% | -85.87% | 3,568 | -8.79% | -41.16% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,657,472 | +0.21% | -46.12% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 97,154 | -5.02% |  | 61 | -3.17% |  | 5,752 | -5.64% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,028,360 | +0.20% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,014 | -0.69% | -58.52% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 100,814 | -4.46% | -65.46% | 33 | -2.94% | -85.27% | 4,552 | -5.01% | -49.65% |
| [Render/strings](#renderstrings) | 1,061,558 | -0.12% | -49.39% | 975 | 0% | -53.88% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,694 | -0.56% | -64.18% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,672 | 0% | -69.69% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,229 | -8.01% | 1,399 | -4.77% | 0 |  | 8,265 | -7.16% |
| Render/chat_llama | 1,247 | -1.27% | 502 | +2.03% | 0 |  | 4,631 | -4.83% |
| Load/chat_mistral | 1,807 | -7.71% | 2,010 | -5.55% | 0 |  | 9,521 | -9.34% |
| Render/chat_mistral | 2,400 | -0.37% | 726 | -2.55% | 0 |  | 13,666 | -2.39% |
| Load/chat_qwen | 850 | -1.05% | 1,245 | -1.58% | 0 |  | 5,426 | -5.39% |
| Render/chat_qwen | 1,167 | -0.17% | 361 | -0.82% | 0 |  | 4,222 | +0.69% |
| Load/config_file | 371 | -4.38% | 554 | +2.78% | 0 |  | 3,423 | -4.49% |
| Render/config_file | 6,684 | -7.91% | 1,983 | -1.29% | 0 |  | 71,045 | +2.46% |
| Load/dict_ops | 85 | -13.27% | 83 | +6.41% | 0 |  | 1,493 | -13.50% |
| Render/dict_ops | 960 | -5.33% | 610 | -1.93% | 0 |  | 1,128 | +1.81% |
| Load/expressions | 132 | -2.94% | 211 | -0.94% | 0 |  | 1,427 | -8.70% |
| Render/expressions | 3 | -57.14% | 1 | -75.00% | 0 |  | 469 | +7.82% |
| Load/filters | 210 | -2.33% | 145 | -2.68% | 0 |  | 2,332 | -0.51% |
| Render/filters | 96 | +3.23% | 72 | +20.00% | 0 |  | 1,190 | +2.32% |
| Load/for_filter_if | 45 | +12.50% | 20 | -31.03% | 0 |  | 1,209 | -6.06% |
| Render/for_filter_if | 1,304 | 0% | 53 | -3.64% | 0 |  | 384 | -10.49% |
| Load/for_loop_vars | 50 | -26.47% | 62 | -11.43% | 0 |  | 1,321 | -4.28% |
| Render/for_loop_vars | 14 | 0% | 6 | 0% | 0 |  | 278 | +5.70% |
| Load/for_range | 26 | +23.81% | 20 | 0% | 0 |  | 940 | -8.91% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 215 | -3.59% |
| Load/html_autoescape | 744 | -2.75% | 900 | -0.88% | 0 |  | 4,598 | -6.12% |
| Render/html_autoescape | 4,741 | +13.91% | 1,711 | +11.18% | 0 |  | 51,651 | +1.76% |
| Load/inheritance | 16 | -11.11% | 17 | 0% | 0 |  | 920 | -9.89% |
| Render/inheritance | 517 | +1.37% | 111 | +13.27% | 0 |  | 1,040 | -31.62% |
| Load/large_static | 2,616 | -0.04% | 1,200 | +0.17% | 0 |  | 430 | +2.63% |
| Render/large_static | 725 | +0.14% | 582 | 0% | 0 |  | 12 | +140.00% |
| Load/macros | 206 | -4.19% | 269 | +2.67% | 0 |  | 1,962 | -3.82% |
| Render/macros | 678 | -4.37% | 210 | -0.94% | 0 |  | 9,868 | +1.85% |
| Load/many_tags | 38,999 | -1.55% | 31,087 | +0.14% | 0 |  | 294,899 | -2.66% |
| Render/many_tags | 7,267 | -2.91% | 1,725 | +0.29% | 0 |  | 4,128 | +717.43% |
| Load/mitsuhiko_table | 94 | -12.15% | 187 | -8.33% | 0 |  | 1,111 | -21.98% |
| Render/mitsuhiko_table | 8,022 | 0% | 5,011 | 0% | 0 |  | 445 | -3.47% |
| Load/mitsuhiko_table_wide | 206 | -5.50% | 334 | +0.91% | 0 |  | 1,562 | -21.59% |
| Render/mitsuhiko_table_wide | 8,072 | -0.15% | 21,459 | -0.01% | 0 |  | 497 | +4.63% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 34 | -17.07% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| Load/strings | 257 | -2.28% | 277 | -1.07% | 0 |  | 3,134 | -5.15% |
| Render/strings | 923 | -0.75% | 1,329 | -1.63% | 0 |  | 3,411 | +74.30% |
| Load/substitute | 8 | +100.00% | 10 | +42.86% | 0 |  | 340 | -6.85% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 12 | +300.00% |

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
