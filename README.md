# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (134 so far, latest [8a2d331](https://github.com/jinja2cpp/Jinja2Cpp/commit/8a2d331a9ba13ea220eebc6efd3fd58cb16ca257) on 2026-10-25). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,446 | +0.02% |  | 55 | 0% |  | 18,256 | -0.26% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 306,172 | +0.05% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 393,218 | -0.01% |  | 52 | 0% |  | 20,728 | -0.23% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 508,043 | -0.10% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 265,907 | -0.02% |  | 44 | 0% |  | 14,400 | -0.33% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 293,544 | 0% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,832 | -0.03% |  | 25 | 0% |  | 5,448 | -0.87% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,258,429 | +0.05% |  | 908 | 0% |  | 23,400 | 0% | -1.71% |
| [Load/dict_ops](#loaddict_ops) | 33,162 | -0.13% | -77.56% | 9 | 0% | -93.13% | 2,192 | -2.14% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 297,186 | +0.11% | -56.98% | 112 | 0% | -75.11% | 24,408 | -0.07% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,364 | -0.07% | -77.99% | 16 | 0% | -89.04% | 2,744 | -1.72% | -55.28% |
| [Render/expressions](#renderexpressions) | 423,479 | 0% | -53.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,819 | -0.03% | -73.83% | 27 | 0% | -88.16% | 3,736 | -1.27% | -67.27% |
| [Render/filters](#renderfilters) | 30,223 | -0.03% | -65.92% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,880 | -0.08% | -77.93% | 7 | 0% | -94.31% | 1,920 | -2.44% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,233 | 0% | -68.43% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,379 | -0.07% | -77.65% | 10 | 0% | -93.63% | 2,400 | -1.96% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,339 | 0% | -82.07% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,570 | -0.22% | -76.06% | 7 | 0% | -87.50% | 1,104 | -4.17% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,750 | -0.02% | -57.98% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,757 | -0.01% |  | 43 | 0% |  | 12,608 | -0.38% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,823 | 0% |  | 674 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 22,745 | -0.13% | -80.07% | 7 | 0% | -93.46% | 1,344 | -3.45% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,499 | -0.03% | -81.75% | 35 | 0% | -94.37% | 6,472 | 0% | +7.58% |
| [Load/int_arith](#loadint_arith) | 66,056 | -0.03% |  | 20 | 0% |  | 3,856 | -1.23% | 0% |
| [Render/int_arith](#renderint_arith) | 822,269 | +0.01% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,234 | -0.02% | -82.48% | 14 | 0% | -96.82% | 43,136 | -0.11% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,880 | 0% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,682 | -0.07% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,259 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,465 | -0.05% | -75.67% | 27 | 0% | -85.48% | 4,376 | -1.08% | -29.60% |
| [Render/macros](#rendermacros) | 650,732 | +0.12% | -74.61% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,080,951 | 0% | -79.44% | 951 | 0% | -97.22% | 431,152 | -0.01% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,441 | 0% | -65.37% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,341 | -0.06% | -80.49% | 19 | 0% | -89.67% | 3,376 | -1.40% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,189,137 | 0% | -58.00% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,116,410 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,188,687 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,000 | -1.16% |  | 35 | 0% |  | 5,704 | -0.83% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,535,194 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,358,489 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,534,731 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,689 | -0.81% | -61.87% | 4 | 0% | -77.78% | 688 | -6.52% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,013 | 0% | -85.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 713 | -0.83% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,281 | -0.02% | -75.23% | 30 | 0% | -86.61% | 4,264 | -1.11% | -52.83% |
| [Render/strings](#renderstrings) | 577,548 | 0% | -72.47% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,712 | -0.39% | -74.17% | 6 | 0% | -83.78% | 848 | -5.36% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,229 | -0.04% | -74.71% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,932 | -0.46% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,034 | -0.05% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 687 | -7.29% | 720 | -0.69% | 0 |  | 7,475 | +0.71% |
| Render/chat_llama | 1,157 | -0.43% | 491 | -0.41% | 0 |  | 3,651 | +1.90% |
| Load/chat_mistral | 1,060 | +1.05% | 1,010 | +0.90% | 0 |  | 8,454 | +1.82% |
| Render/chat_mistral | 1,788 | -2.35% | 541 | -1.64% | 0 |  | 9,365 | +0.69% |
| Load/chat_qwen | 460 | -1.92% | 594 | -0.83% | 0 |  | 4,378 | -2.04% |
| Render/chat_qwen | 1,053 | -0.85% | 383 | -4.73% | 0 |  | 3,085 | -6.71% |
| Load/config_file | 182 | -6.67% | 161 | -3.59% | 0 |  | 2,988 | -2.70% |
| Render/config_file | 5,679 | +2.92% | 1,359 | -4.36% | 0 |  | 37,512 | -3.75% |
| Load/dict_ops | 28 | -28.21% | 17 | -15.00% | 0 |  | 1,335 | +2.46% |
| Render/dict_ops | 730 | +1.53% | 420 | +0.48% | 0 |  | 825 | -5.71% |
| Load/expressions | 22 | -40.54% | 10 | -44.44% | 0 |  | 1,280 | -4.19% |
| Render/expressions | 4 | +33.33% | 1 | 0% | 0 |  | 318 | +2.91% |
| Load/filters | 130 | -14.47% | 70 | -22.22% | 0 |  | 2,193 | +0.46% |
| Render/filters | 14 | -39.13% | 6 | +50.00% | 0 |  | 728 | +3.41% |
| Load/for_filter_if | 12 | -47.83% | 4 | -33.33% | 0 |  | 1,020 | -3.68% |
| Render/for_filter_if | 1,650 | -0.48% | 65 | +3.17% | 0 |  | 176 | +1.73% |
| Load/for_loop_vars | 19 | -48.65% | 11 | -8.33% | 0 |  | 1,051 | -2.32% |
| Render/for_loop_vars | 4 | -66.67% | 1 | 0% | 0 |  | 153 | +4.08% |
| Load/for_range | 2 | -66.67% | 2 | -60.00% | 0 |  | 731 | -2.01% |
| Render/for_range | 1 | 0% | 0 | -100.00% | 0 |  | 114 | -17.39% |
| Load/html_autoescape | 476 | -2.86% | 530 | +2.91% | 0 |  | 3,993 | -3.29% |
| Render/html_autoescape | 4,382 | -13.88% | 1,255 | -21.95% | 0 |  | 33,661 | -0.25% |
| Load/inheritance | 2 | -33.33% | 2 | 0% | 0 |  | 622 | -5.47% |
| Render/inheritance | 568 | -2.57% | 94 | +2.17% | 0 |  | 828 | -3.94% |
| Load/int_arith | 77 | -9.41% | 48 | -5.88% | 0 |  | 2,026 | -1.60% |
| Render/int_arith | 9 | -30.77% | 3 | 0% | 0 |  | 711 | +0.57% |
| Load/large_static | 1,944 | +0.05% | 910 | +0.33% | 0 |  | 320 | +23.08% |
| Render/large_static | 705 | 0% | 570 | 0% | 0 |  | 3 | +50.00% |
| RenderSink/large_static | 0 | -100.00% | 0 |  | 0 |  | 2 | +100.00% |
| RenderTo/large_static | 684 | 0% | 571 | 0% | 0 |  | 2 | 0% |
| Load/macros | 62 | -16.22% | 40 | -31.03% | 0 |  | 1,630 | -0.79% |
| Render/macros | 726 | -0.41% | 214 | -2.73% | 0 |  | 562 | -42.00% |
| Load/many_tags | 27,715 | -1.09% | 22,247 | -0.38% | 0 |  | 257,011 | -1.66% |
| Render/many_tags | 6,122 | -0.36% | 870 | +0.58% | 0 |  | 106 | -37.65% |
| Load/mitsuhiko_table | 15 | -21.05% | 20 | +42.86% | 0 |  | 814 | +3.83% |
| Render/mitsuhiko_table | 9,071 | +0.27% | 6,628 | +0.33% | 0 |  | 291 | -5.21% |
| RenderSink/mitsuhiko_table | 9,012 | -0.03% | 1,493 | +0.54% | 0 |  | 243 | +6.11% |
| RenderTo/mitsuhiko_table | 9,088 | +0.43% | 6,644 | +0.39% | 0 |  | 258 | -1.53% |
| Load/mitsuhiko_table_wide | 87 | -7.45% | 69 | -16.87% | 0 |  | 1,033 | -2.09% |
| Render/mitsuhiko_table_wide | 10,338 | +12.15% | 23,179 | +0.53% | 0 |  | 307 | +0.33% |
| RenderSink/mitsuhiko_table_wide | 9,145 | +0.48% | 1,609 | +2.09% | 0 |  | 247 | +1.23% |
| RenderTo/mitsuhiko_table_wide | 10,333 | +12.02% | 23,132 | +0.50% | 0 |  | 261 | -2.61% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | +220.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 171 | -12.76% | 113 | -5.04% | 0 |  | 2,890 | +1.23% |
| Render/strings | 402 | -5.19% | 455 | +0.22% | 0 |  | 1,504 | +0.13% |
| Load/substitute | 2 | 0% | 2 | -33.33% | 0 |  | 224 | +30.99% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | +100.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |

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

### Load/int_arith

![Load/int_arith](charts/Load_int_arith.svg)

### Render/int_arith

![Render/int_arith](charts/Render_int_arith.svg)

### Load/large_static

![Load/large_static](charts/Load_large_static.svg)

### Render/large_static

![Render/large_static](charts/Render_large_static.svg)

### RenderSink/large_static

![RenderSink/large_static](charts/RenderSink_large_static.svg)

### RenderTo/large_static

![RenderTo/large_static](charts/RenderTo_large_static.svg)

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

### RenderSink/mitsuhiko_table

![RenderSink/mitsuhiko_table](charts/RenderSink_mitsuhiko_table.svg)

### RenderTo/mitsuhiko_table

![RenderTo/mitsuhiko_table](charts/RenderTo_mitsuhiko_table.svg)

### Load/mitsuhiko_table_wide

![Load/mitsuhiko_table_wide](charts/Load_mitsuhiko_table_wide.svg)

### Render/mitsuhiko_table_wide

![Render/mitsuhiko_table_wide](charts/Render_mitsuhiko_table_wide.svg)

### RenderSink/mitsuhiko_table_wide

![RenderSink/mitsuhiko_table_wide](charts/RenderSink_mitsuhiko_table_wide.svg)

### RenderTo/mitsuhiko_table_wide

![RenderTo/mitsuhiko_table_wide](charts/RenderTo_mitsuhiko_table_wide.svg)

### Load/plain_text

![Load/plain_text](charts/Load_plain_text.svg)

### Render/plain_text

![Render/plain_text](charts/Render_plain_text.svg)

### RenderSink/plain_text

![RenderSink/plain_text](charts/RenderSink_plain_text.svg)

### RenderTo/plain_text

![RenderTo/plain_text](charts/RenderTo_plain_text.svg)

### Load/strings

![Load/strings](charts/Load_strings.svg)

### Render/strings

![Render/strings](charts/Render_strings.svg)

### Load/substitute

![Load/substitute](charts/Load_substitute.svg)

### Render/substitute

![Render/substitute](charts/Render_substitute.svg)

### RenderSink/substitute

![RenderSink/substitute](charts/RenderSink_substitute.svg)

### RenderTo/substitute

![RenderTo/substitute](charts/RenderTo_substitute.svg)
