# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (162 so far, latest [56a6a96](https://github.com/jinja2cpp/Jinja2Cpp/commit/56a6a964b07f758f6938240c6e8737cbcb8a7943) on 2026-10-31). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,345 | 0% |  | 55 | 0% |  | 18,240 | 0% | -53.33% |
| [Render/chat_llama](#renderchat_llama) | 310,241 | 0% |  | 251 | 0% |  | 11,712 | 0% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 391,941 | 0% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 515,150 | 0% |  | 424 | 0% |  | 10,064 | 0% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 266,655 | 0% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 295,078 | 0% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 98,590 | 0% |  | 25 | 0% |  | 5,432 | 0% | -48.56% |
| [Render/config_file](#renderconfig_file) | 1,266,643 | 0% |  | 908 | 0% |  | 23,992 | 0% | +0.77% |
| [Load/dict_ops](#loaddict_ops) | 33,098 | 0% | -77.60% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,397 | 0% | -56.95% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,303 | 0% | -78.02% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 424,668 | 0% | -53.13% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,796 | 0% | -73.84% | 27 | 0% | -88.16% | 3,720 | 0% | -67.41% |
| [Render/filters](#renderfilters) | 30,413 | 0% | -65.70% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,853 | 0% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,095 | 0% | -68.33% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,213 | 0% | -77.74% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,696 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,573 | 0% | -76.05% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,790 | 0% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,950 | 0% |  | 43 | 0% |  | 12,592 | 0% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,221,582 | 0% |  | 674 | 0% |  | 45,168 | 0% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,746 | 0% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,638 | 0% | -81.73% | 35 | 0% | -94.37% | 7,320 | 0% | +21.68% |
| [Load/int_arith](#loadint_arith) | 65,837 | 0% |  | 20 | 0% |  | 3,840 | 0% | -0.41% |
| [Render/int_arith](#renderint_arith) | 825,351 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,883 | +0.70% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,246 | +0.12% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,038 | 0% | -82.50% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,888 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,720 | 0% | -75.56% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 650,104 | 0% | -74.64% | 308 | 0% | -88.71% | 20,496 | 0% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,043,070 | 0% | -79.52% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 724,905 | 0% | -65.20% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,495 | 0% | -80.43% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,223,439 | 0% | -57.73% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,150,740 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,223,009 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,059 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,569,157 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,392,459 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,568,721 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,789 | 0% | -75.40% | 30 | 0% | -86.61% | 4,248 | 0% | -53.01% |
| [Render/strings](#renderstrings) | 582,015 | 0% | -72.25% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,715 | 0% | -74.16% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,237 | 0% | -74.62% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,940 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,042 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 738 | 0% | 730 | 0% | 0 |  | 7,499 | -0.70% |
| Render/chat_llama | 1,128 | 0% | 458 | 0% | 0 |  | 4,050 | +12.34% |
| Load/chat_mistral | 1,072 | 0% | 983 | 0% | 0 |  | 8,196 | -1.54% |
| Render/chat_mistral | 1,908 | 0% | 546 | 0% | 0 |  | 9,415 | +2.52% |
| Load/chat_qwen | 495 | 0% | 595 | 0% | 0 |  | 4,438 | -1.00% |
| Render/chat_qwen | 1,158 | 0% | 399 | 0% | 0 |  | 3,364 | +5.19% |
| Load/config_file | 182 | 0% | 165 | 0% | 0 |  | 2,928 | -1.55% |
| Render/config_file | 6,202 | 0% | 1,485 | 0% | 0 |  | 39,788 | -1.40% |
| Load/dict_ops | 33 | 0% | 15 | 0% | 0 |  | 1,281 | -1.61% |
| Render/dict_ops | 749 | 0% | 421 | 0% | 0 |  | 874 | +5.68% |
| Load/expressions | 39 | 0% | 18 | 0% | 0 |  | 1,322 | -0.30% |
| Render/expressions | 5 | 0% | 2 | 0% | 0 |  | 306 | -2.86% |
| Load/filters | 128 | 0% | 66 | 0% | 0 |  | 1,923 | -5.78% |
| Render/filters | 16 | 0% | 3 | 0% | 0 |  | 767 | +1.99% |
| Load/for_filter_if | 20 | 0% | 8 | 0% | 0 |  | 1,024 | -3.67% |
| Render/for_filter_if | 1,742 | 0% | 95 | 0% | 0 |  | 187 | +11.31% |
| Load/for_loop_vars | 23 | 0% | 12 | 0% | 0 |  | 1,006 | -4.01% |
| Render/for_loop_vars | 9 | 0% | 1 | 0% | 0 |  | 174 | -2.25% |
| Load/for_range | 5 | 0% | 5 | 0% | 0 |  | 733 | -0.14% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 106 | -20.30% |
| Load/html_autoescape | 443 | 0% | 514 | 0% | 0 |  | 3,677 | -8.74% |
| Render/html_autoescape | 4,887 | 0% | 1,681 | 0% | 0 |  | 34,502 | -2.17% |
| Load/inheritance | 9 | 0% | 3 | 0% | 0 |  | 651 | +1.24% |
| Render/inheritance | 612 | 0% | 105 | 0% | 0 |  | 867 | -2.03% |
| Load/int_arith | 107 | 0% | 88 | 0% | 0 |  | 2,017 | -4.00% |
| Render/int_arith | 15 | 0% | 4 | 0% | 0 |  | 730 | +1.25% |
| Module/keep | 2 | 0% | 1 | -50.00% | 0 |  | 385 | +5.19% |
| Module/keep_written | 2,932 | 0% | 557 | 0% | 0 |  | 696 | +2.65% |
| Load/large_static | 1,948 | 0% | 910 | 0% | 0 |  | 286 | +4.38% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 11 | +450.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 4 | +100.00% |
| Load/macros | 84 | 0% | 77 | 0% | 0 |  | 1,611 | -0.86% |
| Render/macros | 767 | 0% | 257 | 0% | 0 |  | 3,292 | +44.01% |
| Load/many_tags | 28,971 | 0% | 22,208 | 0% | 0 |  | 237,671 | -8.18% |
| Render/many_tags | 6,119 | 0% | 867 | 0% | 0 |  | 145 | +31.82% |
| Load/mitsuhiko_table | 34 | 0% | 52 | 0% | 0 |  | 815 | -3.09% |
| Render/mitsuhiko_table | 10,082 | 0% | 6,375 | 0% | 0 |  | 385 | +11.59% |
| RenderSink/mitsuhiko_table | 9,959 | 0% | 1,226 | 0% | 0 |  | 320 | +15.94% |
| RenderTo/mitsuhiko_table | 10,027 | 0% | 6,310 | 0% | 0 |  | 349 | +11.50% |
| Load/mitsuhiko_table_wide | 75 | 0% | 94 | 0% | 0 |  | 934 | +1.52% |
| Render/mitsuhiko_table_wide | 10,225 | 0% | 22,698 | 0% | 0 |  | 415 | +13.70% |
| RenderSink/mitsuhiko_table_wide | 10,040 | 0% | 1,309 | 0% | 0 |  | 305 | +7.02% |
| RenderTo/mitsuhiko_table_wide | 10,238 | 0% | 22,687 | 0% | 0 |  | 368 | +10.84% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 22 | +46.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 152 | 0% | 85 | 0% | 0 |  | 2,754 | -1.47% |
| Render/strings | 446 | 0% | 473 | 0% | 0 |  | 1,516 | +1.40% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 203 | +7.98% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |

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
