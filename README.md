# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (158 so far, latest [9895fe6](https://github.com/jinja2cpp/Jinja2Cpp/commit/9895fe6a0a17594f62e828c935006c44ff40a7fb) on 2026-10-30). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,339 | -0.26% |  | 55 | 0% |  | 18,240 | 0% | -53.33% |
| [Render/chat_llama](#renderchat_llama) | 310,508 | +0.14% |  | 251 | 0% |  | 11,712 | 0% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 391,935 | -0.41% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 515,654 | +0.15% |  | 424 | 0% |  | 10,176 | 0% | -22.96% |
| [Load/chat_qwen](#loadchat_qwen) | 266,649 | -0.60% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 295,109 | +0.02% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 98,584 | -0.80% |  | 25 | 0% |  | 5,432 | 0% | -48.56% |
| [Render/config_file](#renderconfig_file) | 1,264,470 | +0.04% |  | 908 | 0% |  | 24,008 | 0% | +0.84% |
| [Load/dict_ops](#loaddict_ops) | 33,092 | -0.23% | -77.61% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,305 | +0.10% | -56.97% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,297 | -0.02% | -78.03% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 424,668 | 0% | -53.13% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,790 | 0% | -73.84% | 27 | 0% | -88.16% | 3,720 | 0% | -67.41% |
| [Render/filters](#renderfilters) | 30,351 | +0.15% | -65.77% | 26 | 0% | -64.86% | 1,096 | 0% | -77.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,847 | -0.79% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,295 | +0.07% | -68.31% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,207 | -0.69% | -77.74% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,696 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,567 | -0.04% | -76.07% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,790 | 0% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,944 | -0.84% |  | 43 | 0% |  | 12,592 | 0% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,221,584 | +0.11% |  | 674 | 0% |  | 45,168 | 0% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,740 | 0% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,688 | +0.03% | -81.72% | 35 | 0% | -94.37% | 7,320 | 0% | +21.68% |
| [Load/int_arith](#loadint_arith) | 65,831 | -0.44% |  | 20 | 0% |  | 3,840 | 0% | -0.41% |
| [Render/int_arith](#renderint_arith) | 824,583 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Module/keep](#modulekeep) | 6,822 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,245 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,032 | 0% | -82.50% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,888 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,688 | -0.34% | -75.57% | 27 | 0% | -85.48% | 4,360 | 0% | -29.86% |
| [Render/macros](#rendermacros) | 650,204 | +0.02% | -74.63% | 308 | 0% | -88.71% | 20,496 | 0% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,043,064 | -0.57% | -79.52% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 724,905 | +0.21% | -65.20% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,489 | 0% | -80.43% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,222,913 | 0% | -57.73% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,150,163 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,222,438 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,053 | -0.01% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,568,910 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,392,215 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,568,477 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,686 | 0% | -61.90% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,783 | -0.31% | -75.40% | 30 | 0% | -86.61% | 4,248 | 0% | -53.01% |
| [Render/strings](#renderstrings) | 581,707 | +0.09% | -72.27% | 344 | 0% | -83.73% | 9,120 | 0% | -49.13% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,237 | 0% | -74.62% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,940 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,042 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 730 | +1.81% | 735 | +0.96% | 0 |  | 8,251 | +13.04% |
| Render/chat_llama | 1,124 | -4.34% | 471 | +0.43% | 0 |  | 3,831 | +8.16% |
| Load/chat_mistral | 1,062 | -1.76% | 988 | -1.20% | 0 |  | 8,778 | +8.33% |
| Render/chat_mistral | 1,923 | +1.00% | 558 | -1.24% | 0 |  | 9,572 | +2.65% |
| Load/chat_qwen | 487 | 0% | 603 | 0% | 0 |  | 4,764 | +10.97% |
| Render/chat_qwen | 1,138 | +2.61% | 412 | +2.74% | 0 |  | 3,278 | +3.64% |
| Load/config_file | 171 | -0.58% | 158 | -4.24% | 0 |  | 3,120 | +3.31% |
| Render/config_file | 6,221 | +5.16% | 1,440 | +1.69% | 0 |  | 39,444 | -3.63% |
| Load/dict_ops | 31 | -6.06% | 16 | -5.88% | 0 |  | 1,353 | +7.21% |
| Render/dict_ops | 758 | +1.07% | 419 | +0.48% | 0 |  | 876 | +1.39% |
| Load/expressions | 46 | +4.55% | 23 | -11.54% | 0 |  | 1,383 | +6.30% |
| Render/expressions | 18 | +50.00% | 6 | +50.00% | 0 |  | 342 | +17.53% |
| Load/filters | 121 | +1.68% | 64 | -4.48% | 0 |  | 2,019 | +4.34% |
| Render/filters | 15 | -16.67% | 3 | -25.00% | 0 |  | 738 | -2.12% |
| Load/for_filter_if | 8 | +14.29% | 3 | 0% | 0 |  | 1,081 | +2.56% |
| Render/for_filter_if | 1,743 | +0.11% | 88 | -4.35% | 0 |  | 178 | +4.09% |
| Load/for_loop_vars | 31 | +10.71% | 16 | -5.88% | 0 |  | 1,089 | +2.54% |
| Render/for_loop_vars | 16 | -20.00% | 1 | 0% | 0 |  | 142 | -27.55% |
| Load/for_range | 8 | +60.00% | 5 | 0% | 0 |  | 758 | +2.57% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 107 | -14.40% |
| Load/html_autoescape | 460 | +0.44% | 512 | +0.20% | 0 |  | 4,039 | +1.99% |
| Render/html_autoescape | 5,000 | +18.43% | 1,627 | +19.99% | 0 |  | 32,567 | -1.16% |
| Load/inheritance | 9 | +12.50% | 3 | 0% | 0 |  | 705 | +7.14% |
| Render/inheritance | 615 | +0.33% | 104 | +2.97% | 0 |  | 913 | +4.46% |
| Load/int_arith | 100 | +4.17% | 86 | -2.27% | 0 |  | 2,149 | +9.81% |
| Render/int_arith | 15 | -6.25% | 5 | +66.67% | 0 |  | 1,966 | +175.74% |
| Module/keep | 1 | 0% | 2 | 0% | 0 |  | 388 | +9.30% |
| Module/keep_written | 2,934 | -0.03% | 556 | -0.18% | 0 |  | 711 | +3.19% |
| Load/large_static | 1,946 | 0% | 910 | 0% | 0 |  | 270 | -6.57% |
| Render/large_static | 709 | -0.42% | 570 | 0% | 0 |  | 3 | -88.46% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 1 | -66.67% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 3 | -40.00% |
| Load/macros | 91 | +21.33% | 75 | +5.63% | 0 |  | 1,689 | +1.26% |
| Render/macros | 778 | -3.95% | 255 | -0.78% | 0 |  | 1,491 | -8.36% |
| Load/many_tags | 29,052 | -0.14% | 22,258 | -0.01% | 0 |  | 255,260 | +1.08% |
| Render/many_tags | 6,121 | -0.03% | 869 | +0.35% | 0 |  | 130 | +9.24% |
| Load/mitsuhiko_table | 43 | +13.16% | 59 | +5.36% | 0 |  | 905 | +13.27% |
| Render/mitsuhiko_table | 10,044 | -0.23% | 6,358 | -0.11% | 0 |  | 335 | +0.90% |
| RenderSink/mitsuhiko_table | 9,915 | -0.14% | 1,208 | -0.82% | 0 |  | 260 | -1.52% |
| RenderTo/mitsuhiko_table | 9,997 | -0.38% | 6,292 | -0.05% | 0 |  | 293 | 0% |
| Load/mitsuhiko_table_wide | 82 | +9.33% | 94 | +3.30% | 0 |  | 947 | +5.93% |
| Render/mitsuhiko_table_wide | 10,207 | -0.47% | 22,683 | +0.02% | 0 |  | 334 | -6.96% |
| RenderSink/mitsuhiko_table_wide | 10,019 | -0.29% | 1,282 | -0.62% | 0 |  | 291 | +16.87% |
| RenderTo/mitsuhiko_table_wide | 10,212 | -0.48% | 22,675 | +0.01% | 0 |  | 327 | -1.51% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 29 | +107.14% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| Load/strings | 148 | -1.99% | 86 | -6.52% | 0 |  | 2,904 | +5.10% |
| Render/strings | 448 | +1.82% | 478 | +0.63% | 0 |  | 1,533 | +2.20% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 181 | -3.72% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | -40.00% |

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
