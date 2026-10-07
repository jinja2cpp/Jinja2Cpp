# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (46 so far, latest [d47d436](https://github.com/jinja2cpp/Jinja2Cpp/commit/d47d4368bec0f016d1951614905b45530960e7e5) on 2026-10-07). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 534,778 | +1.36% |  | 131 | +1.55% |  | 39,000 | 0% | -0.20% |
| [Render/chat_llama](#renderchat_llama) | 410,379 | -1.43% |  | 348 | 0% |  | 13,216 | 0% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 618,425 | +3.00% |  | 123 | +6.03% |  | 38,560 | 0% | -13.64% |
| [Render/chat_mistral](#renderchat_mistral) | 709,591 | -2.02% |  | 475 | +0.21% |  | 11,600 | +0.14% | -12.17% |
| [Load/chat_qwen](#loadchat_qwen) | 394,829 | +1.81% |  | 96 | +2.13% |  | 36,976 | 0% | +35.30% |
| [Render/chat_qwen](#renderchat_qwen) | 396,538 | -1.83% |  | 337 | 0% |  | 7,360 | 0% | -13.45% |
| [Load/config_file](#loadconfig_file) | 134,031 | +2.56% |  | 41 | 0% |  | 9,208 | 0% | -12.80% |
| [Render/config_file](#renderconfig_file) | 1,998,263 | +0.80% |  | 1,704 | -0.35% |  | 21,424 | -0.04% | -10.01% |
| [Load/dict_ops](#loaddict_ops) | 47,389 | +3.91% | -67.93% | 19 | 0% | -85.50% | 4,648 | +0.35% | +9.21% |
| [Render/dict_ops](#renderdict_ops) | 401,978 | -0.67% | -41.81% | 315 | -0.32% | -30.00% | 34,048 | 0% | -1.87% |
| [Load/expressions](#loadexpressions) | 63,048 | +2.40% | -69.42% | 24 | 0% | -83.56% | 8,976 | 0% | +46.28% |
| [Render/expressions](#renderexpressions) | 524,380 | -1.23% | -42.13% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 91,988 | -0.03% | -66.48% | 46 | 0% | -79.82% | 9,504 | 0% | -16.75% |
| [Render/filters](#renderfilters) | 60,248 | +0.74% | -32.05% | 46 | 0% | -37.84% | 4,232 | 0% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 38,213 | +3.81% | -71.78% | 15 | 0% | -87.80% | 4,336 | 0% | +25.75% |
| [Render/for_filter_if](#renderfor_filter_if) | 459,464 | -10.90% | -46.91% | 206 | 0% | -8.85% | 2,552 | 0% | -29.89% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 51,893 | +3.38% | -71.28% | 17 | 0% | -89.17% | 4,408 | 0% | -2.48% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 273,348 | -1.32% | -74.52% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 19,432 | +3.93% | -65.72% | 12 | 0% | -78.57% | 2,224 | 0% | +11.20% |
| [Render/for_range](#renderfor_range) | 46,378 | -2.02% | -49.70% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 260,700 | +2.03% |  | 74 | +1.37% |  | 35,976 | 0% | +45.77% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,680,836 | +0.08% |  | 1,012 | 0% |  | 38,696 | +0.04% | -4.18% |
| [Load/inheritance](#loadinheritance) | 35,382 | +4.22% | -69.00% | 18 | +5.88% | -83.18% | 4,584 | +81.33% | +59.61% |
| [Render/inheritance](#renderinheritance) | 214,167 | +5.64% | -71.36% | 78 | 0% | -87.46% | 3,264 | 0% | -45.74% |
| [Load/large_static](#loadlarge_static) | 219,290 | +0.31% | -75.25% | 28 | 0% | -93.64% | 53,352 | 0% | -0.06% |
| [Render/large_static](#renderlarge_static) | 27,210 | +2.18% | -63.79% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [Load/macros](#loadmacros) | 74,537 | +2.31% | -67.30% | 34 | 0% | -81.72% | 9,368 | 0% | +50.71% |
| [Render/macros](#rendermacros) | 1,168,901 | +1.61% | -54.39% | 407 | 0% | -85.09% | 13,576 | 0% | -7.77% |
| [Load/many_tags](#loadmany_tags) | 12,100,035 | +0.41% | -72.60% | 2,155 | 0% | -93.71% | 801,400 | -0.02% | -24.05% |
| [Render/many_tags](#rendermany_tags) | 1,269,704 | +0.34% | -39.04% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 76,701 | +3.22% | -67.71% | 32 | 0% | -82.61% | 9,152 | +0.18% | +50.92% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,594,287 | -5.44% | -46.63% | 1,025 | 0% | -74.71% | 345,864 | 0% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 96,434 | +2.80% |  | 67 | 0% |  | 11,336 | +0.14% | +37.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,965,429 | -4.91% |  | 1,046 | 0% |  | 1,379,848 | 0% | -0.08% |
| [Load/plain_text](#loadplain_text) | 3,933 | +2.61% | -59.35% | 6 | 0% | -66.67% | 1,152 | 0% | +2.86% |
| [Render/plain_text](#renderplain_text) | 1,022 | +2.82% | -84.96% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 97,164 | +1.83% | -66.71% | 43 | 0% | -80.80% | 9,712 | 0% | +7.43% |
| [Render/strings](#renderstrings) | 1,063,408 | +0.72% | -49.30% | 976 | 0% | -53.83% | 17,248 | 0% | -3.79% |
| [Load/substitute](#loadsubstitute) | 10,354 | +1.25% | -65.32% | 10 | 0% | -72.97% | 2,200 | 0% | +44.74% |
| [Render/substitute](#rendersubstitute) | 2,670 | +2.14% | -69.71% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,187 | +4.77% | 1,283 | +4.91% | 0 |  | 8,281 | +7.57% |
| Render/chat_llama | 1,620 | -2.99% | 518 | -0.96% | 0 |  | 5,316 | +6.15% |
| Load/chat_mistral | 1,423 | +8.38% | 1,664 | +4.98% | 0 |  | 9,179 | +8.56% |
| Render/chat_mistral | 3,161 | -0.09% | 739 | -5.98% | 0 |  | 14,713 | +8.11% |
| Load/chat_qwen | 878 | +6.30% | 1,152 | +0.35% | 0 |  | 4,671 | +2.79% |
| Render/chat_qwen | 1,678 | -2.61% | 368 | -4.66% | 0 |  | 4,451 | +2.06% |
| Load/config_file | 393 | +3.69% | 489 | +8.19% | 0 |  | 3,074 | +5.09% |
| Render/config_file | 10,350 | -5.27% | 2,484 | -0.84% | 0 |  | 75,792 | +7.81% |
| Load/dict_ops | 95 | -5.00% | 72 | +18.03% | 0 |  | 1,543 | +9.74% |
| Render/dict_ops | 1,129 | -3.75% | 625 | +0.16% | 0 |  | 1,132 | -5.51% |
| Load/expressions | 120 | -1.64% | 145 | +9.02% | 0 |  | 1,434 | +15.83% |
| Render/expressions | 14 | -6.67% | 5 | +66.67% | 0 |  | 410 | -12.21% |
| Load/filters | 228 | -2.15% | 139 | +2.21% | 0 |  | 2,180 | +1.02% |
| Render/filters | 168 | +6.33% | 92 | +8.24% | 0 |  | 1,169 | +3.18% |
| Load/for_filter_if | 32 | -13.51% | 22 | -8.33% | 0 |  | 1,058 | +12.55% |
| Render/for_filter_if | 1,261 | +0.88% | 51 | +2.00% | 0 |  | 1,313 | -37.80% |
| Load/for_loop_vars | 68 | +11.48% | 66 | +20.00% | 0 |  | 1,163 | +8.69% |
| Render/for_loop_vars | 24 | 0% | 9 | -10.00% | 0 |  | 290 | 0% |
| Load/for_range | 17 | -19.05% | 16 | -11.11% | 0 |  | 900 | +17.04% |
| Render/for_range | 2 | +100.00% | 1 |  | 0 |  | 219 | +8.42% |
| Load/html_autoescape | 711 | +1.28% | 833 | +4.65% | 0 |  | 3,840 | -1.36% |
| Render/html_autoescape | 8,371 | -8.11% | 1,903 | -10.62% | 0 |  | 52,869 | +2.06% |
| Load/inheritance | 24 | -14.29% | 21 | -16.00% | 0 |  | 952 | +17.39% |
| Render/inheritance | 583 | +9.38% | 97 | -4.90% | 0 |  | 789 | +0.64% |
| Load/large_static | 2,608 | +0.38% | 1,183 | +0.42% | 0 |  | 352 | +6.67% |
| Render/large_static | 823 | +0.37% | 574 | -0.35% | 0 |  | 4 | +33.33% |
| Load/macros | 160 | +5.96% | 176 | +25.71% | 0 |  | 1,713 | +6.00% |
| Render/macros | 688 | +3.15% | 224 | 0% | 0 |  | 9,577 | -23.35% |
| Load/many_tags | 28,084 | +11.54% | 24,213 | +10.22% | 0 |  | 276,712 | +0.71% |
| Render/many_tags | 11,629 | +0.41% | 1,859 | +0.38% | 0 |  | 5,133 | +2.31% |
| Load/mitsuhiko_table | 90 | +4.65% | 139 | +13.93% | 0 |  | 1,208 | +14.29% |
| Render/mitsuhiko_table | 8,054 | -0.05% | 5,121 | 0% | 0 |  | 456 | -16.02% |
| Load/mitsuhiko_table_wide | 219 | +8.42% | 296 | +4.96% | 0 |  | 1,677 | +16.22% |
| Render/mitsuhiko_table_wide | 8,220 | +0.82% | 21,518 | -0.01% | 0 |  | 7,210 | +1160.49% |
| Load/plain_text | 0 |  | 1 |  | 0 |  | 8 | +14.29% |
| Render/plain_text | 0 |  | 1 |  | 0 |  | 4 | 0% |
| Load/strings | 265 | -3.28% | 231 | +5.48% | 0 |  | 2,869 | +6.73% |
| Render/strings | 991 | -5.98% | 1,344 | -0.07% | 0 |  | 2,027 | -5.63% |
| Load/substitute | 4 | +33.33% | 6 | +20.00% | 0 |  | 286 | +2.88% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |

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
