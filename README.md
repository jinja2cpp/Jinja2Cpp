# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (133 so far, latest [28d9a2d](https://github.com/jinja2cpp/Jinja2Cpp/commit/28d9a2d3d6a028959a82efa14f55e8d1d0913e9f) on 2026-10-25). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,388 | -0.69% |  | 55 | 0% |  | 18,304 | 0% | -53.16% |
| [Render/chat_llama](#renderchat_llama) | 306,007 | -0.02% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 393,255 | -0.58% |  | 52 | 0% |  | 20,776 | 0% | -53.47% |
| [Render/chat_mistral](#renderchat_mistral) | 508,554 | -0.04% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 265,960 | -1.02% |  | 44 | 0% |  | 14,448 | 0% | -47.13% |
| [Render/chat_qwen](#renderchat_qwen) | 293,531 | -0.01% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,862 | -0.46% |  | 25 | 0% |  | 5,496 | 0% | -47.95% |
| [Render/config_file](#renderconfig_file) | 1,257,738 | -0.06% |  | 908 | 0% |  | 23,400 | 0% | -1.71% |
| [Load/dict_ops](#loaddict_ops) | 33,206 | -0.38% | -77.53% | 9 | 0% | -93.13% | 2,240 | 0% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 296,855 | -0.02% | -57.03% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,394 | -0.25% | -77.98% | 16 | 0% | -89.04% | 2,792 | 0% | -54.50% |
| [Render/expressions](#renderexpressions) | 423,479 | -0.05% | -53.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,843 | -0.28% | -73.82% | 27 | 0% | -88.16% | 3,784 | 0% | -66.85% |
| [Render/filters](#renderfilters) | 30,231 | -0.01% | -65.91% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,903 | -0.79% | -77.91% | 7 | 0% | -94.31% | 1,968 | 0% | -42.92% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,234 | +0.36% | -68.43% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,409 | -0.67% | -77.63% | 10 | 0% | -93.63% | 2,448 | 0% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,340 | -0.05% | -82.07% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,600 | -0.64% | -76.01% | 7 | 0% | -87.50% | 1,152 | 0% | -42.40% |
| [Render/for_range](#renderfor_range) | 38,757 | -0.26% | -57.97% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,775 | -0.41% |  | 43 | 0% |  | 12,656 | 0% | -48.72% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,882 | -0.06% |  | 674 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 22,774 | -0.58% | -80.04% | 7 | 0% | -93.46% | 1,392 | 0% | -51.53% |
| [Render/inheritance](#renderinheritance) | 136,544 | -0.20% | -81.74% | 35 | 0% | -94.37% | 6,472 | 0% | +7.58% |
| [Load/int_arith](#loadint_arith) | 66,077 | -0.41% |  | 20 | 0% |  | 3,904 | 0% | +1.24% |
| [Render/int_arith](#renderint_arith) | 822,204 | -0.02% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,264 | -0.17% | -82.48% | 14 | 0% | -96.82% | 43,184 | 0% | -19.11% |
| [Render/large_static](#renderlarge_static) | 20,881 | -0.02% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,691 | -0.03% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | -0.02% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,494 | -0.38% | -75.66% | 27 | 0% | -85.48% | 4,424 | 0% | -28.83% |
| [Render/macros](#rendermacros) | 649,933 | +0.14% | -74.64% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,080,648 | -0.59% | -79.44% | 951 | 0% | -97.22% | 431,184 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,435 | -0.13% | -65.37% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,371 | -0.22% | -80.48% | 19 | 0% | -89.67% | 3,424 | 0% | -43.54% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,189,138 | -0.21% | -58.00% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,116,419 | -0.21% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,188,688 | -0.21% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,646 | -0.17% |  | 35 | 0% |  | 5,752 | 0% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,535,184 | -0.20% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,358,487 | -0.20% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,534,721 | -0.20% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,719 | -1.01% | -61.56% | 4 | 0% | -77.78% | 736 | 0% | -34.29% |
| [Render/plain_text](#renderplain_text) | 1,013 | -0.39% | -85.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 719 | -0.55% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | -0.49% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,299 | -0.35% | -75.23% | 30 | 0% | -86.61% | 4,312 | 0% | -52.30% |
| [Render/strings](#renderstrings) | 577,543 | -0.17% | -72.47% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,742 | -0.57% | -74.07% | 6 | 0% | -83.78% | 896 | 0% | -41.05% |
| [Render/substitute](#rendersubstitute) | 2,230 | -0.18% | -74.70% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,941 | -0.21% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | -0.20% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 741 | +0.82% | 725 | +0.97% | 0 |  | 7,422 | -4.97% |
| Render/chat_llama | 1,162 | +0.78% | 493 | +0.82% | 0 |  | 3,583 | -3.79% |
| Load/chat_mistral | 1,049 | -0.29% | 1,001 | -0.99% | 0 |  | 8,303 | -5.48% |
| Render/chat_mistral | 1,831 | -0.05% | 550 | +2.04% | 0 |  | 9,301 | +4.44% |
| Load/chat_qwen | 469 | -1.26% | 599 | -1.32% | 0 |  | 4,469 | -4.61% |
| Render/chat_qwen | 1,062 | +1.24% | 402 | +1.26% | 0 |  | 3,307 | +5.99% |
| Load/config_file | 195 | +4.28% | 167 | +3.73% | 0 |  | 3,071 | -2.79% |
| Render/config_file | 5,518 | -4.25% | 1,421 | +1.00% | 0 |  | 38,972 | -2.72% |
| Load/dict_ops | 39 | -13.33% | 20 | 0% | 0 |  | 1,303 | -2.10% |
| Render/dict_ops | 719 | +0.14% | 418 | -0.71% | 0 |  | 875 | +3.80% |
| Load/expressions | 37 | -19.57% | 18 | -14.29% | 0 |  | 1,336 | -0.37% |
| Render/expressions | 3 | 0% | 1 | 0% | 0 |  | 309 | +22.62% |
| Load/filters | 152 | +14.29% | 90 | +11.11% | 0 |  | 2,183 | +2.25% |
| Render/filters | 23 | -23.33% | 4 | -50.00% | 0 |  | 704 | -4.22% |
| Load/for_filter_if | 23 | +15.00% | 6 | -14.29% | 0 |  | 1,059 | -6.78% |
| Render/for_filter_if | 1,658 | +0.18% | 63 | -1.56% | 0 |  | 173 | -9.42% |
| Load/for_loop_vars | 37 | +23.33% | 12 | +20.00% | 0 |  | 1,076 | -10.26% |
| Render/for_loop_vars | 12 | +200.00% | 1 | 0% | 0 |  | 147 | +2.80% |
| Load/for_range | 6 | +100.00% | 5 | +66.67% | 0 |  | 746 | +2.05% |
| Render/for_range | 1 |  | 1 | 0% | 0 |  | 138 | +15.97% |
| Load/html_autoescape | 490 | -0.20% | 515 | -1.15% | 0 |  | 4,129 | -5.77% |
| Render/html_autoescape | 5,088 | +0.20% | 1,608 | -2.43% | 0 |  | 33,744 | +1.80% |
| Load/inheritance | 3 | +50.00% | 2 | 0% | 0 |  | 658 | -0.30% |
| Render/inheritance | 583 | +2.10% | 92 | +1.10% | 0 |  | 862 | +0.58% |
| Load/int_arith | 85 | -2.30% | 51 | -12.07% | 0 |  | 2,059 | -1.10% |
| Render/int_arith | 13 | +62.50% | 3 | 0% | 0 |  | 707 | -0.70% |
| Load/large_static | 1,943 | +0.26% | 907 | -0.22% | 0 |  | 260 | -6.47% |
| Render/large_static | 705 | 0% | 570 | 0% | 0 |  | 2 | -33.33% |
| RenderSink/large_static | 1 |  | 0 |  | 0 |  | 1 | 0% |
| RenderTo/large_static | 684 | 0% | 571 | 0% | 0 |  | 2 | -33.33% |
| Load/macros | 74 | +10.45% | 58 | +3.57% | 0 |  | 1,643 | -5.52% |
| Render/macros | 729 | +0.55% | 220 | +1.85% | 0 |  | 969 | +40.84% |
| Load/many_tags | 28,021 | +0.44% | 22,331 | +0.15% | 0 |  | 261,357 | +1.23% |
| Render/many_tags | 6,144 | -0.03% | 865 | -0.92% | 0 |  | 170 | +20.57% |
| Load/mitsuhiko_table | 19 | -20.83% | 14 | -33.33% | 0 |  | 784 | -14.04% |
| Render/mitsuhiko_table | 9,047 | +0.02% | 6,606 | -0.06% | 0 |  | 307 | -6.69% |
| RenderSink/mitsuhiko_table | 9,015 | -0.03% | 1,485 | -0.40% | 0 |  | 229 | -8.40% |
| RenderTo/mitsuhiko_table | 9,049 | +0.09% | 6,618 | +0.05% | 0 |  | 262 | -7.75% |
| Load/mitsuhiko_table_wide | 94 | +3.30% | 83 | -7.78% | 0 |  | 1,055 | +7.87% |
| Render/mitsuhiko_table_wide | 9,218 | -0.10% | 23,057 | 0% | 0 |  | 306 | -3.16% |
| RenderSink/mitsuhiko_table_wide | 9,101 | -0.03% | 1,576 | +0.25% | 0 |  | 244 | +4.72% |
| RenderTo/mitsuhiko_table_wide | 9,224 | +0.22% | 23,016 | -0.03% | 0 |  | 268 | -5.96% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 5 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 196 | +8.89% | 119 | -2.46% | 0 |  | 2,855 | -1.25% |
| Render/strings | 424 | +1.92% | 454 | +0.44% | 0 |  | 1,502 | +2.32% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 171 | +9.62% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 1 | 0% |
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
