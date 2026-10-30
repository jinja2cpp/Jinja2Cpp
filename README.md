# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (159 so far, latest [a9402fa](https://github.com/jinja2cpp/Jinja2Cpp/commit/a9402fa7b52982cc08edd47d2910e6a454b38695) on 2026-10-30). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,345 | 0% |  | 55 | 0% |  | 18,240 | 0% | -53.33% |
| [Render/chat_llama](#renderchat_llama) | 310,238 | -0.09% |  | 251 | 0% |  | 11,712 | 0% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 391,941 | 0% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 515,158 | -0.10% |  | 424 | 0% |  | 10,064 | -1.10% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 266,655 | 0% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 295,109 | 0% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 98,590 | +0.01% |  | 25 | 0% |  | 5,432 | 0% | -48.56% |
| [Render/config_file](#renderconfig_file) | 1,266,604 | +0.17% |  | 908 | 0% |  | 23,992 | -0.07% | +0.77% |
| [Load/dict_ops](#loaddict_ops) | 33,098 | +0.02% | -77.60% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,397 | +0.03% | -56.95% | 112 | 0% | -75.11% | 24,408 | -0.07% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,303 | +0.01% | -78.02% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 424,668 | 0% | -53.13% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,796 | +0.01% | -73.84% | 27 | 0% | -88.16% | 3,720 | 0% | -67.41% |
| [Render/filters](#renderfilters) | 30,413 | +0.20% | -65.70% | 26 | 0% | -64.86% | 1,080 | -1.46% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,853 | +0.02% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,295 | 0% | -68.31% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,213 | +0.01% | -77.74% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,696 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,573 | +0.04% | -76.05% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,790 | 0% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,950 | 0% |  | 43 | 0% |  | 12,592 | 0% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,221,663 | +0.01% |  | 674 | 0% |  | 45,168 | 0% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,746 | +0.03% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,688 | 0% | -81.72% | 35 | 0% | -94.37% | 7,320 | 0% | +21.68% |
| [Load/int_arith](#loadint_arith) | 65,837 | +0.01% |  | 20 | 0% |  | 3,840 | 0% | -0.41% |
| [Render/int_arith](#renderint_arith) | 825,351 | +0.09% |  | 12 | 0% |  | 3,272 | -0.49% | -0.49% |
| [Module/keep](#modulekeep) | 6,822 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,191 | -0.15% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,038 | 0% | -82.50% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,888 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,720 | +0.06% | -75.56% | 27 | 0% | -85.48% | 4,344 | -0.37% | -30.12% |
| [Render/macros](#rendermacros) | 650,204 | 0% | -74.63% | 308 | 0% | -88.71% | 20,496 | 0% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,043,070 | 0% | -79.52% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 724,905 | 0% | -65.20% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,495 | +0.01% | -80.43% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,224,428 | +0.03% | -57.72% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,151,729 | +0.03% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,223,998 | +0.03% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,059 | +0.01% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,570,146 | +0.02% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,393,448 | +0.02% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,569,710 | +0.02% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | +0.16% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,789 | +0.01% | -75.40% | 30 | 0% | -86.61% | 4,248 | 0% | -53.01% |
| [Render/strings](#renderstrings) | 582,015 | +0.05% | -72.25% | 344 | 0% | -83.73% | 9,088 | -0.35% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,715 | +0.08% | -74.16% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,237 | 0% | -74.62% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,940 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,042 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 730 | 0% | 734 | -0.14% | 0 |  | 7,574 | -8.21% |
| Render/chat_llama | 1,117 | -0.62% | 466 | -1.06% | 0 |  | 3,551 | -7.31% |
| Load/chat_mistral | 1,064 | +0.19% | 990 | +0.20% | 0 |  | 8,089 | -7.85% |
| Render/chat_mistral | 1,917 | -0.31% | 552 | -1.08% | 0 |  | 9,250 | -3.36% |
| Load/chat_qwen | 479 | -1.64% | 600 | -0.50% | 0 |  | 4,303 | -9.68% |
| Render/chat_qwen | 1,115 | -2.02% | 408 | -0.97% | 0 |  | 3,207 | -2.17% |
| Load/config_file | 173 | +1.17% | 157 | -0.63% | 0 |  | 2,947 | -5.54% |
| Render/config_file | 6,441 | +3.54% | 1,452 | +0.83% | 0 |  | 39,937 | +1.25% |
| Load/dict_ops | 30 | -3.23% | 16 | 0% | 0 |  | 1,295 | -4.29% |
| Render/dict_ops | 759 | +0.13% | 421 | +0.48% | 0 |  | 858 | -2.05% |
| Load/expressions | 41 | -10.87% | 24 | +4.35% | 0 |  | 1,244 | -10.05% |
| Render/expressions | 11 | -38.89% | 4 | -33.33% | 0 |  | 334 | -2.34% |
| Load/filters | 122 | +0.83% | 64 | 0% | 0 |  | 2,123 | +5.15% |
| Render/filters | 17 | +13.33% | 2 | -33.33% | 0 |  | 766 | +3.79% |
| Load/for_filter_if | 15 | +87.50% | 5 | +66.67% | 0 |  | 1,054 | -2.50% |
| Render/for_filter_if | 1,740 | -0.17% | 89 | +1.14% | 0 |  | 200 | +12.36% |
| Load/for_loop_vars | 25 | -19.35% | 17 | +6.25% | 0 |  | 1,062 | -2.48% |
| Render/for_loop_vars | 15 | -6.25% | 1 | 0% | 0 |  | 160 | +12.68% |
| Load/for_range | 5 | -37.50% | 5 | 0% | 0 |  | 719 | -5.15% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 95 | -11.21% |
| Load/html_autoescape | 468 | +1.74% | 512 | 0% | 0 |  | 3,949 | -2.23% |
| Render/html_autoescape | 4,639 | -7.22% | 1,671 | +2.70% | 0 |  | 35,078 | +7.71% |
| Load/inheritance | 11 | +22.22% | 4 | +33.33% | 0 |  | 669 | -5.11% |
| Render/inheritance | 613 | -0.33% | 104 | 0% | 0 |  | 882 | -3.40% |
| Load/int_arith | 101 | +1.00% | 95 | +10.47% | 0 |  | 1,969 | -8.38% |
| Render/int_arith | 17 | +13.33% | 3 | -40.00% | 0 |  | 755 | -61.60% |
| Module/keep | 2 | +100.00% | 2 | 0% | 0 |  | 365 | -5.93% |
| Module/keep_written | 2,932 | -0.07% | 557 | +0.18% | 0 |  | 681 | -4.22% |
| Load/large_static | 1,947 | +0.05% | 910 | 0% | 0 |  | 249 | -7.78% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 3 | 0% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 1 | 0% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 3 | 0% |
| Load/macros | 86 | -5.49% | 74 | -1.33% | 0 |  | 1,633 | -3.32% |
| Render/macros | 771 | -0.90% | 257 | +0.78% | 0 |  | 2,539 | +70.29% |
| Load/many_tags | 28,973 | -0.27% | 22,260 | +0.01% | 0 |  | 254,463 | -0.31% |
| Render/many_tags | 6,119 | -0.03% | 873 | +0.46% | 0 |  | 125 | -3.85% |
| Load/mitsuhiko_table | 39 | -9.30% | 53 | -10.17% | 0 |  | 845 | -6.63% |
| Render/mitsuhiko_table | 10,062 | +0.18% | 6,365 | +0.11% | 0 |  | 351 | +4.78% |
| RenderSink/mitsuhiko_table | 9,938 | +0.23% | 1,217 | +0.75% | 0 |  | 266 | +2.31% |
| RenderTo/mitsuhiko_table | 10,011 | +0.14% | 6,301 | +0.14% | 0 |  | 311 | +6.14% |
| Load/mitsuhiko_table_wide | 78 | -4.88% | 89 | -5.32% | 0 |  | 999 | +5.49% |
| Render/mitsuhiko_table_wide | 10,186 | -0.21% | 22,690 | +0.03% | 0 |  | 351 | +5.09% |
| RenderSink/mitsuhiko_table_wide | 10,013 | -0.06% | 1,287 | +0.39% | 0 |  | 259 | -11.00% |
| RenderTo/mitsuhiko_table_wide | 10,188 | -0.24% | 22,687 | +0.05% | 0 |  | 307 | -6.12% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | -51.72% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +66.67% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| Load/strings | 151 | +2.03% | 87 | +1.16% | 0 |  | 2,783 | -4.17% |
| Render/strings | 443 | -1.12% | 479 | +0.21% | 0 |  | 1,499 | -2.22% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 173 | -4.42% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | -33.33% |

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
