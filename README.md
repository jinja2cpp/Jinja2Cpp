# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (157 so far, latest [8f4d4f5](https://github.com/jinja2cpp/Jinja2Cpp/commit/8f4d4f51a9ddac8945019a6f4124863fade6f7f6) on 2026-10-30). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,237 | +0.08% |  | 55 | 0% |  | 18,240 | -0.09% | -53.33% |
| [Render/chat_llama](#renderchat_llama) | 310,071 | +0.07% |  | 251 | 0% |  | 11,712 | 0% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 393,539 | +0.06% |  | 52 | 0% |  | 20,680 | -0.08% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 514,891 | +0.18% |  | 424 | 0% |  | 10,176 | +0.16% | -22.96% |
| [Load/chat_qwen](#loadchat_qwen) | 268,261 | +0.22% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 295,058 | +0.02% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 99,375 | +0.10% |  | 25 | 0% |  | 5,432 | -0.29% | -48.56% |
| [Render/config_file](#renderconfig_file) | 1,263,985 | +0.08% |  | 908 | 0% |  | 24,008 | 0% | +0.84% |
| [Load/dict_ops](#loaddict_ops) | 33,168 | +0.05% | -77.56% | 9 | 0% | -93.13% | 2,176 | -0.73% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,005 | +0.30% | -57.01% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,307 | -0.03% | -78.02% | 16 | 0% | -89.04% | 2,728 | -0.58% | -55.54% |
| [Render/expressions](#renderexpressions) | 424,668 | +0.01% | -53.13% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,790 | +0.05% | -73.84% | 27 | 0% | -88.16% | 3,720 | -0.43% | -67.41% |
| [Render/filters](#renderfilters) | 30,305 | -0.01% | -65.82% | 26 | 0% | -64.86% | 1,096 | 0% | -77.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,086 | +0.07% | -77.78% | 7 | 0% | -94.31% | 1,904 | -0.83% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,095 | 0% | -68.33% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,488 | +0.02% | -77.59% | 10 | 0% | -93.63% | 2,368 | -0.67% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,696 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,573 | 0% | -76.05% | 7 | 0% | -87.50% | 1,088 | -1.45% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,790 | +0.07% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,587 | +0.08% |  | 43 | 0% |  | 12,592 | -0.13% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,220,299 | +0.13% |  | 674 | 0% |  | 45,168 | 0% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,740 | -0.03% | -80.07% | 7 | 0% | -93.46% | 1,328 | -1.19% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,647 | +0.02% | -81.73% | 35 | 0% | -94.37% | 7,320 | 0% | +21.68% |
| [Load/int_arith](#loadint_arith) | 66,123 | +0.05% |  | 20 | 0% |  | 3,840 | -0.41% | -0.41% |
| [Render/int_arith](#renderint_arith) | 824,580 | +0.01% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Module/keep](#modulekeep) | 6,822 | -0.42% |  | 7 | -12.50% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,245 | +0.06% |  | 11 | -8.33% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,032 | 0% | -82.50% | 14 | 0% | -96.82% | 43,120 | -0.04% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,888 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,876 | +0.04% | -75.49% | 27 | 0% | -85.48% | 4,360 | -0.37% | -29.86% |
| [Render/macros](#rendermacros) | 650,104 | 0% | -74.64% | 308 | 0% | -88.71% | 20,496 | 0% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,095,264 | +0.07% | -79.41% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 723,405 | 0% | -65.27% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,489 | -0.01% | -80.43% | 19 | 0% | -89.67% | 3,360 | -0.47% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,222,913 | 0% | -57.73% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,150,163 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,222,438 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,059 | 0% |  | 35 | 0% |  | 5,688 | -0.28% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,568,910 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,392,215 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,568,477 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,686 | -0.16% | -61.90% | 4 | 0% | -77.78% | 672 | -2.33% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,003 | +0.04% | -75.33% | 30 | 0% | -86.61% | 4,248 | -0.38% | -53.01% |
| [Render/strings](#renderstrings) | 581,158 | 0% | -72.29% | 344 | 0% | -83.73% | 9,120 | 0% | -49.13% |
| [Load/substitute](#loadsubstitute) | 7,709 | -0.08% | -74.18% | 6 | 0% | -83.78% | 832 | -1.89% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,237 | 0% | -74.62% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,940 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,042 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 717 | -0.97% | 728 | +0.55% | 0 |  | 7,299 | -3.94% |
| Render/chat_llama | 1,175 | +4.72% | 469 | -1.88% | 0 |  | 3,542 | -1.34% |
| Load/chat_mistral | 1,081 | -3.91% | 1,000 | +0.20% | 0 |  | 8,103 | -3.17% |
| Render/chat_mistral | 1,904 | +2.81% | 565 | +4.05% | 0 |  | 9,325 | +2.48% |
| Load/chat_qwen | 487 | -3.56% | 603 | -0.17% | 0 |  | 4,293 | -4.49% |
| Render/chat_qwen | 1,109 | +2.40% | 401 | +3.89% | 0 |  | 3,163 | +0.64% |
| Load/config_file | 172 | -4.97% | 165 | -0.60% | 0 |  | 3,020 | 0% |
| Render/config_file | 5,916 | +2.18% | 1,416 | -1.53% | 0 |  | 40,929 | -0.19% |
| Load/dict_ops | 33 | -2.94% | 17 | -15.00% | 0 |  | 1,262 | -2.92% |
| Render/dict_ops | 750 | -2.85% | 417 | -0.24% | 0 |  | 864 | -1.03% |
| Load/expressions | 44 | -22.81% | 26 | -23.53% | 0 |  | 1,301 | +1.56% |
| Render/expressions | 12 | +200.00% | 4 | +100.00% | 0 |  | 291 | -2.68% |
| Load/filters | 119 | -10.53% | 67 | -2.90% | 0 |  | 1,935 | -3.92% |
| Render/filters | 18 | -18.18% | 4 | -33.33% | 0 |  | 754 | +0.13% |
| Load/for_filter_if | 7 | 0% | 3 | -40.00% | 0 |  | 1,054 | +0.38% |
| Render/for_filter_if | 1,741 | +0.64% | 92 | +4.55% | 0 |  | 171 | -18.57% |
| Load/for_loop_vars | 28 | -6.67% | 17 | +30.77% | 0 |  | 1,062 | -2.39% |
| Render/for_loop_vars | 20 | +53.85% | 1 | -66.67% | 0 |  | 196 | +12.64% |
| Load/for_range | 5 | +25.00% | 5 | +66.67% | 0 |  | 739 | +0.54% |
| Render/for_range | 1 | -75.00% | 1 | -50.00% | 0 |  | 125 | +35.87% |
| Load/html_autoescape | 458 | +0.88% | 511 | +1.79% | 0 |  | 3,960 | +0.28% |
| Render/html_autoescape | 4,222 | -6.72% | 1,356 | -2.02% | 0 |  | 32,948 | -1.05% |
| Load/inheritance | 8 | +100.00% | 3 | 0% | 0 |  | 658 | -0.60% |
| Render/inheritance | 613 | +2.85% | 101 | +5.21% | 0 |  | 874 | -2.35% |
| Load/int_arith | 96 | -20.00% | 88 | -5.38% | 0 |  | 1,957 | -1.36% |
| Render/int_arith | 16 | 0% | 3 | -40.00% | 0 |  | 713 | -2.73% |
| Module/keep | 1 | -66.67% | 2 | +100.00% | 0 |  | 355 | +12.34% |
| Module/keep_written | 2,935 | +0.41% | 557 | +0.54% | 0 |  | 689 | +9.37% |
| Load/large_static | 1,946 | +0.21% | 910 | -0.11% | 0 |  | 289 | +20.42% |
| Render/large_static | 712 | +0.42% | 570 | 0% | 0 |  | 26 | +420.00% |
| RenderSink/large_static | 0 | -100.00% | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 5 | 0% |
| Load/macros | 75 | -7.41% | 71 | -1.39% | 0 |  | 1,668 | +1.96% |
| Render/macros | 810 | +5.74% | 257 | -1.15% | 0 |  | 1,627 | +35.58% |
| Load/many_tags | 29,094 | -5.16% | 22,260 | -0.49% | 0 |  | 252,526 | -0.83% |
| Render/many_tags | 6,123 | +0.10% | 866 | +0.12% | 0 |  | 119 | -42.51% |
| Load/mitsuhiko_table | 38 | -13.64% | 56 | 0% | 0 |  | 799 | -5.56% |
| Render/mitsuhiko_table | 10,067 | +0.27% | 6,365 | +0.05% | 0 |  | 332 | -5.14% |
| RenderSink/mitsuhiko_table | 9,929 | +0.08% | 1,218 | +0.83% | 0 |  | 264 | +2.33% |
| RenderTo/mitsuhiko_table | 10,035 | +0.24% | 6,295 | +0.03% | 0 |  | 293 | -4.87% |
| Load/mitsuhiko_table_wide | 75 | -1.32% | 91 | -5.21% | 0 |  | 894 | -8.31% |
| Render/mitsuhiko_table_wide | 10,255 | +0.35% | 22,679 | -0.04% | 0 |  | 359 | +2.87% |
| RenderSink/mitsuhiko_table_wide | 10,048 | +0.04% | 1,290 | +0.23% | 0 |  | 249 | -0.80% |
| RenderTo/mitsuhiko_table_wide | 10,261 | +0.39% | 22,673 | -0.01% | 0 |  | 332 | +12.16% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | +133.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -42.86% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | -42.86% |
| Load/strings | 151 | -11.70% | 92 | -6.12% | 0 |  | 2,763 | -0.29% |
| Render/strings | 440 | -0.45% | 475 | +0.42% | 0 |  | 1,500 | -36.55% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 188 | +50.40% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | 0% |

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

### Module/keep

![Module/keep](charts/Module_keep.svg)

### Module/keep_written

![Module/keep_written](charts/Module_keep_written.svg)

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
