# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (66 so far, latest [0ae6057](https://github.com/jinja2cpp/Jinja2Cpp/commit/0ae6057b6d48bde3b23c737e559899c47a480333) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 554,418 | -0.14% |  | 92 | 0% |  | 19,856 | 0% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 405,775 | -0.37% |  | 355 | +0.57% |  | 15,312 | +0.90% | +6.63% |
| [Load/chat_mistral](#loadchat_mistral) | 657,404 | -0.03% |  | 75 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 643,059 | -0.61% |  | 421 | +0.48% |  | 12,184 | +0.86% | -7.75% |
| [Load/chat_qwen](#loadchat_qwen) | 424,216 | -0.07% |  | 68 | 0% |  | 15,760 | 0% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 387,943 | -1.55% |  | 320 | -5.60% |  | 9,400 | +18.69% | +10.54% |
| [Load/config_file](#loadconfig_file) | 145,994 | +0.02% |  | 34 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,690,298 | -2.75% |  | 1,124 | -4.66% |  | 25,312 | +5.85% | +6.32% |
| [Load/dict_ops](#loaddict_ops) | 47,666 | -0.08% | -67.74% | 12 | 0% | -90.84% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 350,090 | -0.37% | -49.32% | 114 | 0% | -74.67% | 36,168 | 0% | +4.24% |
| [Load/expressions](#loadexpressions) | 67,090 | -0.15% | -67.46% | 18 | 0% | -87.67% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 517,323 | 0% | -42.91% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 97,381 | -0.28% | -64.52% | 32 | 0% | -85.96% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,811 | +0.02% | -40.44% | 37 | 0% | -50.00% | 2,568 | +0.63% | -47.72% |
| [Load/for_filter_if](#loadfor_filter_if) | 41,136 | +0.07% | -69.62% | 9 | 0% | -92.68% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 340,600 | -10.49% | -60.65% | 6 | -97.06% | -97.35% | 13,904 | +389.58% | +281.98% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 57,369 | +0.22% | -68.24% | 13 | 0% | -91.72% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,737 | +0.15% | -74.58% | 4 | 0% | -99.23% | 2,128 | +0.76% | -23.12% |
| [Load/for_range](#loadfor_range) | 19,570 | -0.12% | -65.48% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,279 | 0% | -49.81% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 289,372 | -0.09% |  | 61 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,563,404 | -2.22% |  | 938 | -7.40% |  | 45,760 | +10.00% | +13.31% |
| [Load/inheritance](#loadinheritance) | 34,739 | +0.15% | -69.56% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 149,869 | -22.61% | -79.96% | 34 | -58.02% | -94.53% | 7,176 | +62.50% | +19.28% |
| [Load/large_static](#loadlarge_static) | 236,744 | -0.39% | -73.28% | 28 | 0% | -93.64% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,471 | -0.07% | -71.42% | 3 | 0% | -83.33% | 37,112 | 0% | -0.32% |
| [Load/macros](#loadmacros) | 81,622 | -0.13% | -64.19% | 32 | 0% | -82.80% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 984,002 | -2.15% | -61.61% | 312 | -23.90% | -88.57% | 20,272 | +37.64% | +37.72% |
| [Load/many_tags](#loadmany_tags) | 13,295,266 | -0.12% | -69.90% | 962 | 0% | -97.19% | 445,680 | 0% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,138,879 | -2.32% | -45.33% | 15 | 0% | -28.57% | 82,520 | 0% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 78,041 | -0.09% | -67.15% | 26 | 0% | -85.87% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,557,215 | -1.41% | -46.93% | 35 | -96.59% | -99.14% | 373,872 | +7.92% | +7.74% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 96,410 | -0.06% |  | 57 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,927,680 | -1.33% |  | 53 | -94.93% |  | 1,407,264 | +1.95% | +1.90% |
| [Load/plain_text](#loadplain_text) | 4,058 | -0.25% | -58.06% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 986 | 0% | -85.49% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 101,339 | -0.06% | -65.28% | 33 | 0% | -85.27% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,055,690 | +0.07% | -49.67% | 979 | 0% | -53.69% | 18,384 | +0.09% | +2.54% |
| [Load/substitute](#loadsubstitute) | 10,806 | -0.54% | -63.81% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,684 | -1.00% | -69.55% | 3 | 0% | -75.00% | 600 | 0% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,187 | -0.17% | 1,386 | -0.14% | 0 |  | 8,584 | -2.32% |
| Render/chat_llama | 1,293 | +1.25% | 508 | +1.80% | 0 |  | 4,562 | +0.35% |
| Load/chat_mistral | 1,799 | +0.39% | 2,061 | -0.82% | 0 |  | 9,889 | -4.32% |
| Render/chat_mistral | 2,173 | -0.09% | 544 | +2.45% | 0 |  | 13,049 | +0.25% |
| Load/chat_qwen | 838 | +1.33% | 1,233 | -0.56% | 0 |  | 5,479 | -5.89% |
| Render/chat_qwen | 1,242 | +6.98% | 395 | +3.67% | 0 |  | 4,397 | +4.97% |
| Load/config_file | 378 | -0.53% | 565 | +0.18% | 0 |  | 3,564 | -1.98% |
| Render/config_file | 7,361 | +3.78% | 2,110 | +4.87% | 0 |  | 62,454 | -8.08% |
| Load/dict_ops | 95 | -7.77% | 111 | +7.77% | 0 |  | 1,598 | -0.75% |
| Render/dict_ops | 960 | +1.16% | 730 | -0.27% | 0 |  | 1,080 | +1.41% |
| Load/expressions | 118 | -4.84% | 208 | 0% | 0 |  | 1,487 | -3.75% |
| Render/expressions | 8 | -20.00% | 2 | 0% | 0 |  | 476 | +11.21% |
| Load/filters | 200 | -2.44% | 146 | -3.95% | 0 |  | 2,558 | -4.23% |
| Render/filters | 60 | +22.45% | 35 | +16.67% | 0 |  | 1,133 | -2.50% |
| Load/for_filter_if | 41 | -18.00% | 35 | -20.45% | 0 |  | 1,150 | -5.97% |
| Render/for_filter_if | 1,591 | +22.57% | 68 | +11.48% | 0 |  | 271 | +11.07% |
| Load/for_loop_vars | 55 | +1.85% | 75 | -5.06% | 0 |  | 1,356 | -3.07% |
| Render/for_loop_vars | 13 | +30.00% | 8 | +33.33% | 0 |  | 259 | -4.78% |
| Load/for_range | 20 | -23.08% | 28 | 0% | 0 |  | 943 | -0.63% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 221 | +8.87% |
| Load/html_autoescape | 649 | -0.15% | 884 | +0.23% | 0 |  | 5,013 | -1.34% |
| Render/html_autoescape | 5,423 | +34.17% | 1,499 | +0.33% | 0 |  | 49,446 | +0.81% |
| Load/inheritance | 16 | -30.43% | 18 | -21.74% | 0 |  | 933 | -2.71% |
| Render/inheritance | 584 | +14.73% | 126 | +26.00% | 0 |  | 885 | -25.51% |
| Load/large_static | 2,618 | -0.15% | 1,190 | 0% | 0 |  | 467 | -3.71% |
| Render/large_static | 711 | 0% | 579 | 0% | 0 |  | 3 | -75.00% |
| Load/macros | 201 | -5.63% | 274 | -0.36% | 0 |  | 2,014 | -1.18% |
| Render/macros | 784 | +15.12% | 228 | +10.14% | 0 |  | 5,464 | -11.37% |
| Load/many_tags | 39,623 | -0.52% | 32,827 | -1.36% | 0 |  | 300,906 | -6.53% |
| Render/many_tags | 6,881 | -0.03% | 1,458 | -0.48% | 0 |  | 1,518 | -64.42% |
| Load/mitsuhiko_table | 115 | +0.88% | 202 | 0% | 0 |  | 1,199 | -7.05% |
| Render/mitsuhiko_table | 9,313 | +16.14% | 6,503 | +29.67% | 0 |  | 520 | +19.54% |
| Load/mitsuhiko_table_wide | 192 | -1.54% | 334 | -0.30% | 0 |  | 1,837 | +8.25% |
| Render/mitsuhiko_table_wide | 9,418 | +16.43% | 22,995 | +7.12% | 0 |  | 540 | +10.66% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 7 | -66.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| Load/strings | 256 | +1.59% | 292 | +0.69% | 0 |  | 3,200 | -1.42% |
| Render/strings | 928 | +0.54% | 1,344 | -0.37% | 0 |  | 1,965 | -49.94% |
| Load/substitute | 8 | +14.29% | 14 | +16.67% | 0 |  | 370 | -1.60% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 3 | -40.00% |

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
