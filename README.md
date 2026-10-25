# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (141 so far, latest [955a130](https://github.com/jinja2cpp/Jinja2Cpp/commit/955a130880080b995d15f946c544cfbe3979b147) on 2026-10-26). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,584 | +0.04% |  | 55 | 0% |  | 18,256 | 0% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 306,077 | -0.03% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 393,472 | +0.06% |  | 52 | 0% |  | 20,728 | 0% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 508,192 | +0.02% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,166 | +0.10% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 293,467 | -0.02% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,923 | +0.09% |  | 25 | 0% |  | 5,448 | 0% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,258,116 | -0.07% |  | 908 | 0% |  | 23,400 | 0% | -1.71% |
| [Load/dict_ops](#loaddict_ops) | 33,176 | +0.02% | -77.55% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,100 | -0.47% | -57.14% | 112 | 0% | -75.11% | 24,424 | +0.07% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,363 | 0% | -77.99% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 423,485 | 0% | -53.26% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,813 | 0% | -73.83% | 27 | 0% | -88.16% | 3,736 | 0% | -67.27% |
| [Render/filters](#renderfilters) | 30,231 | +0.01% | -65.91% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,908 | +0.09% | -77.91% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,230 | 0% | -68.43% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,414 | +0.09% | -77.63% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,338 | 0% | -82.07% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,570 | 0% | -76.06% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,756 | +0.01% | -57.97% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,953 | +0.10% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,820 | 0% |  | 674 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 22,745 | 0% | -80.07% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,499 | -0.01% | -81.75% | 35 | 0% | -94.37% | 6,472 | 0% | +7.58% |
| [Load/int_arith](#loadint_arith) | 66,083 | +0.04% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 824,164 | +0.10% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,234 | 0% | -82.48% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,881 | 0% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,683 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,487 | +0.04% | -75.66% | 27 | 0% | -85.48% | 4,376 | 0% | -29.60% |
| [Render/macros](#rendermacros) | 649,931 | -0.12% | -74.64% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,087,251 | +0.07% | -79.42% | 951 | 0% | -97.22% | 431,152 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,442 | 0% | -65.37% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,335 | -0.01% | -80.50% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,188 | -0.04% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,461 | -0.04% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,738 | -0.04% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,988 | -0.02% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,177 | -0.04% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,488 | -0.04% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,746 | -0.04% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,689 | 0% | -61.87% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,014 | 0% | -85.07% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 714 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 815 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,284 | +0.03% | -75.23% | 30 | 0% | -86.61% | 4,264 | 0% | -52.83% |
| [Render/strings](#renderstrings) | 577,971 | +0.07% | -72.44% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,712 | 0% | -74.17% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,230 | 0% | -74.70% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,933 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 710 | +1.57% | 726 | +0.28% | 0 |  | 7,301 | -2.74% |
| Render/chat_llama | 1,140 | +0.09% | 479 | -1.24% | 0 |  | 3,647 | +1.70% |
| Load/chat_mistral | 1,070 | +0.19% | 1,008 | 0% | 0 |  | 8,370 | -1.43% |
| Render/chat_mistral | 1,814 | -1.84% | 557 | +0.18% | 0 |  | 8,935 | -0.66% |
| Load/chat_qwen | 463 | -2.32% | 592 | -0.17% | 0 |  | 4,534 | +1.34% |
| Render/chat_qwen | 1,030 | -1.44% | 388 | -1.02% | 0 |  | 3,036 | -1.62% |
| Load/config_file | 184 | +6.98% | 168 | +7.01% | 0 |  | 3,024 | -0.30% |
| Render/config_file | 5,574 | -5.04% | 1,416 | -2.28% | 0 |  | 41,600 | +7.51% |
| Load/dict_ops | 31 | -3.12% | 18 | +5.88% | 0 |  | 1,317 | -1.94% |
| Render/dict_ops | 754 | +2.86% | 416 | -0.24% | 0 |  | 849 | +2.29% |
| Load/expressions | 25 | -7.41% | 14 | 0% | 0 |  | 1,268 | -2.24% |
| Render/expressions | 2 | -50.00% | 1 | 0% | 0 |  | 317 | -1.25% |
| Load/filters | 131 | -3.68% | 73 | +2.82% | 0 |  | 1,972 | -9.00% |
| Render/filters | 13 | -51.85% | 3 | -50.00% | 0 |  | 751 | +2.32% |
| Load/for_filter_if | 8 | -11.11% | 4 | 0% | 0 |  | 1,032 | -0.77% |
| Render/for_filter_if | 1,659 | +0.36% | 58 | -6.45% | 0 |  | 186 | -0.53% |
| Load/for_loop_vars | 17 | -22.73% | 8 | 0% | 0 |  | 1,089 | +0.93% |
| Render/for_loop_vars | 8 | +14.29% | 1 | -50.00% | 0 |  | 163 | +18.12% |
| Load/for_range | 2 | -33.33% | 3 | 0% | 0 |  | 728 | +0.83% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 150 | +45.63% |
| Load/html_autoescape | 481 | +2.12% | 527 | 0% | 0 |  | 4,183 | +5.90% |
| Render/html_autoescape | 3,634 | -13.50% | 1,157 | -6.32% | 0 |  | 34,830 | +2.82% |
| Load/inheritance | 2 | -33.33% | 2 | 0% | 0 |  | 631 | -4.25% |
| Render/inheritance | 577 | +1.23% | 89 | 0% | 0 |  | 829 | -2.93% |
| Load/int_arith | 76 | -8.43% | 48 | -5.88% | 0 |  | 1,952 | -4.22% |
| Render/int_arith | 15 | -6.25% | 5 | +25.00% | 0 |  | 683 | +3.96% |
| Load/large_static | 1,944 | +0.10% | 910 | +0.11% | 0 |  | 323 | +10.24% |
| Render/large_static | 706 | 0% | 568 | 0% | 0 |  | 11 | +10.00% |
| RenderSink/large_static | 0 |  | 1 | 0% | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 687 | 0% | 568 | 0% | 0 |  | 3 | -25.00% |
| Load/macros | 68 | +9.68% | 47 | +17.50% | 0 |  | 1,633 | -1.92% |
| Render/macros | 739 | +0.68% | 220 | +1.38% | 0 |  | 2,030 | +275.23% |
| Load/many_tags | 28,114 | +1.49% | 22,375 | +0.58% | 0 |  | 253,197 | +0.77% |
| Render/many_tags | 6,130 | +0.15% | 862 | -0.35% | 0 |  | 194 | +10.86% |
| Load/mitsuhiko_table | 19 | 0% | 20 | 0% | 0 |  | 796 | -5.01% |
| Render/mitsuhiko_table | 9,042 | -0.02% | 6,607 | -0.17% | 0 |  | 341 | +25.37% |
| RenderSink/mitsuhiko_table | 9,014 | +0.12% | 1,493 | -0.40% | 0 |  | 262 | +28.43% |
| RenderTo/mitsuhiko_table | 9,046 | -0.12% | 6,609 | -0.30% | 0 |  | 298 | +21.14% |
| Load/mitsuhiko_table_wide | 85 | 0% | 70 | +2.94% | 0 |  | 1,014 | +6.40% |
| Render/mitsuhiko_table_wide | 9,233 | -1.81% | 23,072 | -0.03% | 0 |  | 327 | +13.94% |
| RenderSink/mitsuhiko_table_wide | 9,113 | -0.04% | 1,583 | -0.13% | 0 |  | 268 | +29.47% |
| RenderTo/mitsuhiko_table_wide | 9,222 | -1.84% | 23,002 | -0.10% | 0 |  | 288 | +10.34% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | +133.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | -14.29% |
| Load/strings | 176 | +2.33% | 119 | +6.25% | 0 |  | 2,895 | +0.38% |
| Render/strings | 433 | -0.23% | 455 | -0.44% | 0 |  | 2,472 | +6.09% |
| Load/substitute | 2 | 0% | 2 | 0% | 0 |  | 238 | +34.46% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |

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
