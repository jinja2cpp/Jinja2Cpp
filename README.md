# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (152 so far, latest [a64c314](https://github.com/jinja2cpp/Jinja2Cpp/commit/a64c3148bef4492df8b25f50b3d4f1ce898f4948) on 2026-10-28). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,192 | +0.06% |  | 55 | 0% |  | 18,256 | 0% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 309,434 | +0.63% |  | 251 | 0% |  | 11,712 | +5.17% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 394,246 | +0.05% |  | 52 | 0% |  | 20,728 | 0% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 514,383 | +0.81% |  | 424 | 0% |  | 10,144 | +6.55% | -23.20% |
| [Load/chat_qwen](#loadchat_qwen) | 267,172 | +0.03% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 295,366 | +0.14% |  | 238 | 0% |  | 8,304 | +6.13% | -2.35% |
| [Load/config_file](#loadconfig_file) | 99,317 | +0.06% |  | 25 | 0% |  | 5,448 | 0% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,263,095 | +0.40% |  | 908 | 0% |  | 24,008 | +2.53% | +0.84% |
| [Load/dict_ops](#loaddict_ops) | 33,184 | -0.01% | -77.54% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,098 | 0% | -57.14% | 112 | 0% | -75.11% | 24,440 | +0.07% | -29.56% |
| [Load/expressions](#loadexpressions) | 45,367 | 0% | -77.99% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 424,641 | 0% | -53.14% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,816 | -0.01% | -73.83% | 27 | 0% | -88.16% | 3,736 | 0% | -67.27% |
| [Render/filters](#renderfilters) | 30,350 | +0.25% | -65.77% | 26 | 0% | -64.86% | 1,096 | +1.48% | -77.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,075 | 0% | -77.79% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,692 | +0.43% | -68.26% | 4 | 0% | -98.23% | 16,560 | +24.25% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,520 | 0% | -77.57% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,685 | +0.18% | -82.04% | 2 | 0% | -99.62% | 1,568 | +1.03% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,573 | 0% | -76.05% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,763 | -0.03% | -57.96% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,615 | +0.02% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,218,532 | +0.17% |  | 674 | 0% |  | 45,168 | +3.26% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,746 | -0.01% | -80.07% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,858 | +0.16% | -81.70% | 35 | 0% | -94.37% | 7,320 | +12.82% | +21.68% |
| [Load/int_arith](#loadint_arith) | 66,087 | +0.01% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 824,188 | -0.22% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Module/keep](#modulekeep) | 6,836 |  |  | 8 |  |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,183 |  |  | 12 |  |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,253 | +0.01% | -82.48% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,888 | +0.02% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | +0.03% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | +0.02% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,710 | +0.09% | -75.56% | 27 | 0% | -85.48% | 4,376 | 0% | -29.60% |
| [Render/macros](#rendermacros) | 650,662 | +0.11% | -74.61% | 308 | 0% | -88.71% | 20,496 | +8.65% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,087,219 | 0% | -79.42% | 951 | 0% | -97.22% | 431,152 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 723,277 | +0.25% | -65.28% | 2 | 0% | -90.48% | 5,648 | +0.28% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,344 | 0% | -80.49% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,222,755 | +0.69% | -57.73% | 33 | 0% | -99.19% | 381,536 | +2.21% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,149,947 | +0.69% |  | 32 | 0% |  | 37,064 | +28.59% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,222,211 | +0.68% |  | 32 | 0% |  | 37,064 | +28.59% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,003 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,568,907 | +0.65% |  | 33 | 0% |  | 1,414,928 | +0.59% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,392,198 | +0.67% |  | 32 | 0% |  | 37,064 | +28.59% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,568,460 | +0.65% |  | 32 | 0% |  | 37,064 | +28.59% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,021 | +0.39% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | +0.59% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | +0.51% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,273 | -0.04% | -75.24% | 30 | 0% | -86.61% | 4,264 | 0% | -52.83% |
| [Render/strings](#renderstrings) | 580,476 | +0.43% | -72.33% | 344 | 0% | -83.73% | 9,120 | +0.18% | -49.13% |
| [Load/substitute](#loadsubstitute) | 7,715 | 0% | -74.16% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,237 | +0.18% | -74.62% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,940 | +0.22% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,042 | +0.21% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 715 | -3.25% | 738 | +0.82% | 0 |  | 7,522 | +0.25% |
| Render/chat_llama | 1,135 | -0.09% | 468 | -4.10% | 0 |  | 3,579 | +0.87% |
| Load/chat_mistral | 1,059 | -6.28% | 1,009 | -1.27% | 0 |  | 8,258 | -1.81% |
| Render/chat_mistral | 1,854 | +2.49% | 562 | +3.12% | 0 |  | 9,382 | -1.52% |
| Load/chat_qwen | 479 | -0.83% | 591 | -0.34% | 0 |  | 4,327 | -2.30% |
| Render/chat_qwen | 1,122 | +5.35% | 385 | -3.99% | 0 |  | 3,199 | +1.39% |
| Load/config_file | 169 | -9.63% | 162 | -1.22% | 0 |  | 2,937 | -4.98% |
| Render/config_file | 6,044 | +1.17% | 1,440 | +7.54% | 0 |  | 38,715 | +1.85% |
| Load/dict_ops | 38 | -17.39% | 20 | -16.67% | 0 |  | 1,320 | -2.73% |
| Render/dict_ops | 740 | 0% | 423 | +1.93% | 0 |  | 855 | +2.64% |
| Load/expressions | 28 | +16.67% | 12 | -7.69% | 0 |  | 1,349 | -0.88% |
| Render/expressions | 2 | 0% | 1 | 0% | 0 |  | 300 | +1.01% |
| Load/filters | 134 | -9.46% | 69 | -12.66% | 0 |  | 1,998 | -2.68% |
| Render/filters | 22 | -4.35% | 8 | +60.00% | 0 |  | 752 | +4.01% |
| Load/for_filter_if | 9 | -10.00% | 4 | 0% | 0 |  | 1,048 | -2.87% |
| Render/for_filter_if | 1,720 | +3.74% | 82 | +38.98% | 0 |  | 177 | +1.72% |
| Load/for_loop_vars | 23 | +15.00% | 12 | -7.69% | 0 |  | 1,062 | -5.85% |
| Render/for_loop_vars | 14 | -6.67% | 2 | 0% | 0 |  | 143 | +6.72% |
| Load/for_range | 5 | +150.00% | 5 | +66.67% | 0 |  | 737 | -3.91% |
| Render/for_range | 1 | 0% | 0 | -100.00% | 0 |  | 105 | -5.41% |
| Load/html_autoescape | 458 | -1.72% | 522 | -1.14% | 0 |  | 3,669 | -14.30% |
| Render/html_autoescape | 4,388 | +9.18% | 1,405 | +17.18% | 0 |  | 35,166 | +4.84% |
| Load/inheritance | 3 | +50.00% | 2 | 0% | 0 |  | 647 | -5.55% |
| Render/inheritance | 583 | +2.28% | 102 | +6.25% | 0 |  | 887 | +4.97% |
| Load/int_arith | 86 | +16.22% | 51 | +6.25% | 0 |  | 2,013 | -4.37% |
| Render/int_arith | 11 | +22.22% | 2 | -33.33% | 0 |  | 734 | +2.23% |
| Module/keep | 2 |  | 1 |  | 0 |  | 295 |  |
| Module/keep_written | 2,924 |  | 556 |  | 0 |  | 670 |  |
| Load/large_static | 1,947 | +0.26% | 910 | +0.11% | 0 |  | 230 | -12.55% |
| Render/large_static | 706 | 0% | 570 | +0.18% | 0 |  | 37 | +825.00% |
| RenderSink/large_static | 0 | -100.00% | 0 | -100.00% | 0 |  | 5 | +66.67% |
| RenderTo/large_static | 684 | -0.29% | 571 | +0.35% | 0 |  | 7 | +133.33% |
| Load/macros | 63 | +6.78% | 51 | +15.91% | 0 |  | 1,597 | -5.84% |
| Render/macros | 777 | +5.57% | 233 | +6.88% | 0 |  | 1,956 | +251.80% |
| Load/many_tags | 28,144 | -3.49% | 22,223 | -1.99% | 0 |  | 243,724 | -6.04% |
| Render/many_tags | 6,122 | -0.23% | 854 | -0.70% | 0 |  | 340 | +115.19% |
| Load/mitsuhiko_table | 24 | +26.32% | 31 | +82.35% | 0 |  | 789 | -7.72% |
| Render/mitsuhiko_table | 10,018 | +10.29% | 6,309 | -4.74% | 0 |  | 325 | +14.04% |
| RenderSink/mitsuhiko_table | 9,927 | +9.91% | 1,197 | -19.45% | 0 |  | 266 | +15.15% |
| RenderTo/mitsuhiko_table | 10,034 | +10.46% | 6,285 | -4.95% | 0 |  | 295 | +12.17% |
| Load/mitsuhiko_table_wide | 83 | +10.67% | 74 | +17.46% | 0 |  | 943 | -0.74% |
| Render/mitsuhiko_table_wide | 10,234 | +10.48% | 22,731 | -1.52% | 0 |  | 335 | +11.30% |
| RenderSink/mitsuhiko_table_wide | 10,076 | +10.08% | 1,279 | -20.11% | 0 |  | 274 | +25.11% |
| RenderTo/mitsuhiko_table_wide | 10,219 | +10.46% | 22,675 | -1.46% | 0 |  | 315 | +19.77% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 21 | -32.26% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 9 | +80.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 6 | +50.00% |
| RenderTo/plain_text | 0 |  | 0 | -100.00% | 0 |  | 8 | +60.00% |
| Load/strings | 160 | -11.60% | 120 | -1.64% | 0 |  | 2,827 | -1.43% |
| Render/strings | 425 | -0.23% | 455 | -0.66% | 0 |  | 1,517 | +3.13% |
| Load/substitute | 1 | -50.00% | 4 | +33.33% | 0 |  | 127 | -27.01% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 21 | +425.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 13 | +333.33% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 13 | +333.33% |

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
