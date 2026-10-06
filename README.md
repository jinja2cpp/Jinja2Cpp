# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (41 so far, latest [85fd9a2](https://github.com/jinja2cpp/Jinja2Cpp/commit/85fd9a2395c747c93f56397f3e94755853d2730d) on 2026-10-06). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 650,471 | -0.32% |  | 390 | 0% |  | 31,160 | +0.41% | -20.27% |
| [Render/chat_llama](#renderchat_llama) | 420,352 | -0.42% |  | 348 | -0.29% |  | 13,216 | -0.78% | -7.97% |
| [Load/chat_mistral](#loadchat_mistral) | 760,876 | -0.11% |  | 442 | 0% |  | 38,608 | +0.33% | -13.53% |
| [Render/chat_mistral](#renderchat_mistral) | 740,479 | -0.35% |  | 503 | -0.20% |  | 11,616 | -1.02% | -12.05% |
| [Load/chat_qwen](#loadchat_qwen) | 485,566 | -0.05% |  | 291 | 0% |  | 24,608 | +0.59% | -9.95% |
| [Render/chat_qwen](#renderchat_qwen) | 416,367 | -0.54% |  | 358 | -0.28% |  | 7,360 | -1.39% | -13.45% |
| [Load/config_file](#loadconfig_file) | 163,609 | 0% |  | 116 | 0% |  | 9,064 | +1.43% | -14.17% |
| [Render/config_file](#renderconfig_file) | 2,012,165 | -5.50% |  | 1,770 | -4.07% |  | 21,416 | -0.22% | -10.05% |
| [Load/dict_ops](#loaddict_ops) | 51,492 | -1.15% | -65.16% | 42 | 0% | -67.94% | 3,376 | +3.94% | -20.68% |
| [Render/dict_ops](#renderdict_ops) | 404,195 | -0.63% | -41.49% | 316 | -0.32% | -29.78% | 34,016 | -0.35% | -1.96% |
| [Load/expressions](#loadexpressions) | 74,021 | +0.07% | -64.09% | 62 | 0% | -57.53% | 5,368 | +2.13% | -12.52% |
| [Render/expressions](#renderexpressions) | 531,401 | -0.59% | -41.36% | 3 | -25.00% | -90.00% | 2,472 | -4.04% | -30.25% |
| [Load/filters](#loadfilters) | 117,625 | +0.33% | -57.14% | 106 | 0% | -53.51% | 6,288 | +2.08% | -44.92% |
| [Render/filters](#renderfilters) | 60,004 | -3.31% | -32.33% | 46 | -2.13% | -37.84% | 4,232 | -2.40% | -13.84% |
| [Load/for_filter_if](#loadfor_filter_if) | 42,262 | -0.69% | -68.78% | 36 | 0% | -70.73% | 3,016 | +4.43% | -12.53% |
| [Render/for_filter_if](#renderfor_filter_if) | 513,150 | -0.32% | -40.71% | 206 | -0.48% | -8.85% | 2,552 | -3.92% | -29.89% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 59,590 | -0.08% | -67.02% | 48 | 0% | -69.43% | 3,928 | +3.37% | -13.10% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 602,396 | -0.55% | -43.84% | 502 | -0.20% | -3.83% | 1,624 | -6.02% | -41.33% |
| [Load/for_range](#loadfor_range) | 20,469 | -1.07% | -63.89% | 20 | 0% | -64.29% | 1,648 | +8.42% | -17.60% |
| [Render/for_range](#renderfor_range) | 47,441 | -3.23% | -48.55% | 3 | -25.00% | -88.89% | 440 | -19.12% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 342,274 | -0.17% |  | 254 | 0% |  | 20,912 | +0.54% | -15.27% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,769,368 | -1.65% |  | 1,072 | -0.19% |  | 38,664 | -0.06% | -4.26% |
| [Load/inheritance](#loadinheritance) | 37,608 | -0.59% | -67.04% | 33 | 0% | -69.16% | 2,568 | +5.25% | -10.58% |
| [Render/inheritance](#renderinheritance) | 202,716 | -26.86% | -72.89% | 78 | -49.68% | -87.46% | 3,248 | -15.06% | -46.01% |
| [Load/large_static](#loadlarge_static) | 287,501 | 0% | -67.55% | 178 | 0% | -59.55% | 48,472 | +0.26% | -9.20% |
| [Render/large_static](#renderlarge_static) | 26,780 | -12.01% | -64.36% | 1 | -50.00% | -94.44% | 36,552 | -0.28% | -1.83% |
| [Load/macros](#loadmacros) | 86,268 | -0.21% | -62.16% | 72 | 0% | -61.29% | 5,464 | +2.40% | -12.10% |
| [Render/macros](#rendermacros) | 1,146,734 | -0.34% | -55.26% | 407 | -0.25% | -85.09% | 13,576 | -0.76% | -7.77% |
| [Load/many_tags](#loadmany_tags) | 16,121,785 | -0.06% | -63.50% | 10,238 | 0% | -70.11% | 784,040 | +0.02% | -25.70% |
| [Render/many_tags](#rendermany_tags) | 1,256,971 | -0.41% | -39.66% | 2 | -33.33% | -90.48% | 5,632 | -1.81% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 87,350 | +0.49% | -63.23% | 72 | 0% | -60.87% | 5,392 | +2.43% | -11.08% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,971,770 | -0.34% | -43.58% | 1,025 | -0.10% | -74.71% | 345,864 | -0.03% | -0.33% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 108,032 | +0.12% |  | 107 | 0% |  | 7,576 | +1.72% | -8.33% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 7,322,859 | -0.32% |  | 1,046 | -0.10% |  | 1,379,848 | -0.01% | -0.08% |
| [Load/plain_text](#loadplain_text) | 4,121 | -3.88% | -57.41% | 8 | 0% | -55.56% | 848 | +17.78% | -24.29% |
| [Render/plain_text](#renderplain_text) | 995 | -57.38% | -85.36% | 1 | -50.00% | -91.67% | 40 | -72.22% | -94.44% |
| [Load/strings](#loadstrings) | 112,922 | -0.20% | -61.31% | 90 | 0% | -59.82% | 6,680 | +1.95% | -26.11% |
| [Render/strings](#renderstrings) | 1,106,871 | -0.20% | -47.23% | 1,084 | -0.09% | -48.72% | 17,248 | -0.60% | -3.79% |
| [Load/substitute](#loadsubstitute) | 11,103 | -1.85% | -62.81% | 15 | 0% | -59.46% | 1,088 | +13.33% | -28.42% |
| [Render/substitute](#rendersubstitute) | 2,617 | -35.39% | -70.31% | 1 | -50.00% | -91.67% | 40 | -72.22% | -94.44% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,811 | -0.49% | 1,246 | -1.11% | 0 |  | 9,643 | +2.75% |
| Render/chat_llama | 1,773 | -0.11% | 482 | -5.30% | 0 |  | 4,809 | -1.94% |
| Load/chat_mistral | 2,177 | +0.14% | 1,723 | +1.47% | 0 |  | 11,265 | +4.35% |
| Render/chat_mistral | 3,452 | -0.12% | 761 | -5.70% | 0 |  | 13,905 | -1.29% |
| Load/chat_qwen | 1,316 | -0.83% | 1,209 | +1.00% | 0 |  | 5,805 | +0.64% |
| Render/chat_qwen | 1,836 | -3.67% | 383 | +8.19% | 0 |  | 4,482 | +2.33% |
| Load/config_file | 628 | +1.95% | 533 | +0.76% | 0 |  | 3,628 | +2.25% |
| Render/config_file | 12,372 | -1.57% | 2,541 | +0.16% | 0 |  | 68,259 | -11.50% |
| Load/dict_ops | 138 | 0% | 84 | -1.18% | 0 |  | 1,528 | -0.78% |
| Render/dict_ops | 1,093 | +1.67% | 616 | +1.15% | 0 |  | 1,201 | -6.90% |
| Load/expressions | 199 | -2.93% | 173 | -10.82% | 0 |  | 1,439 | +0.70% |
| Render/expressions | 9 | -73.53% | 3 | -76.92% | 0 |  | 436 | -24.17% |
| Load/filters | 326 | +3.16% | 148 | +2.07% | 0 |  | 2,646 | +9.98% |
| Render/filters | 169 | -15.50% | 79 | -22.55% | 0 |  | 1,124 | -9.28% |
| Load/for_filter_if | 82 | -7.87% | 39 | -11.36% | 0 |  | 1,043 | +5.67% |
| Render/for_filter_if | 1,256 | -5.99% | 52 | -16.13% | 0 |  | 423 | -77.81% |
| Load/for_loop_vars | 120 | +5.26% | 89 | +11.25% | 0 |  | 1,243 | +6.79% |
| Render/for_loop_vars | 777 | +836.14% | 8 | -20.00% | 0 |  | 2,258 | -35.13% |
| Load/for_range | 37 | +5.71% | 28 | -3.45% | 0 |  | 830 | +0.85% |
| Render/for_range | 5 | -54.55% | 2 | -50.00% | 0 |  | 235 | -38.16% |
| Load/html_autoescape | 1,090 | -2.24% | 798 | -0.25% | 0 |  | 5,252 | +6.60% |
| Render/html_autoescape | 12,485 | -7.53% | 2,887 | +21.05% | 0 |  | 59,251 | -1.36% |
| Load/inheritance | 43 | +79.17% | 27 | +22.73% | 0 |  | 899 | +1.24% |
| Render/inheritance | 550 | -9.24% | 102 | -18.40% | 0 |  | 1,060 | +4.13% |
| Load/large_static | 2,759 | -3.43% | 1,209 | +0.17% | 0 |  | 406 | +9.43% |
| Render/large_static | 832 | -5.35% | 576 | -1.03% | 0 |  | 4 | -80.95% |
| Load/macros | 258 | -6.86% | 243 | +1.67% | 0 |  | 1,841 | +3.60% |
| Render/macros | 667 | -2.06% | 220 | +9.45% | 0 |  | 4,165 | -43.69% |
| Load/many_tags | 52,267 | +2.60% | 24,227 | -3.44% | 0 |  | 315,374 | +2.08% |
| Render/many_tags | 11,831 | -0.19% | 1,868 | +23.22% | 0 |  | 1,212 | -69.99% |
| Load/mitsuhiko_table | 183 | +8.93% | 201 | -1.47% | 0 |  | 1,155 | -3.27% |
| Render/mitsuhiko_table | 8,062 | -0.41% | 5,122 | -1.31% | 0 |  | 578 | -12.56% |
| Load/mitsuhiko_table_wide | 316 | -0.32% | 337 | -2.03% | 0 |  | 1,815 | +4.91% |
| Render/mitsuhiko_table_wide | 8,125 | -0.28% | 21,518 | +0.26% | 0 |  | 10,332 | +1353.16% |
| Load/plain_text | 0 | -100.00% | 0 | -100.00% | 0 |  | 18 | +63.64% |
| Render/plain_text | 0 | -100.00% | 0 |  | 0 |  | 7 | 0% |
| Load/strings | 412 | -1.90% | 265 | -2.21% | 0 |  | 3,109 | +2.14% |
| Render/strings | 938 | -6.29% | 1,333 | -3.05% | 0 |  | 3,441 | +16.25% |
| Load/substitute | 4 | -33.33% | 4 | -60.00% | 0 |  | 255 | -8.27% |
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
