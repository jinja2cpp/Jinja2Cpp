# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (160 so far, latest [adf1572](https://github.com/jinja2cpp/Jinja2Cpp/commit/adf1572e1b2912140633d4e369cbfdbef2bf3ddf) on 2026-10-30). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,345 | 0% |  | 55 | 0% |  | 18,240 | 0% | -53.33% |
| [Render/chat_llama](#renderchat_llama) | 310,084 | -0.05% |  | 251 | 0% |  | 11,712 | 0% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 391,941 | 0% |  | 52 | 0% |  | 20,680 | 0% | -53.68% |
| [Render/chat_mistral](#renderchat_mistral) | 515,109 | -0.01% |  | 424 | 0% |  | 10,064 | 0% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 266,655 | 0% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 295,109 | 0% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 98,590 | 0% |  | 25 | 0% |  | 5,432 | 0% | -48.56% |
| [Render/config_file](#renderconfig_file) | 1,266,604 | 0% |  | 908 | 0% |  | 23,992 | 0% | +0.77% |
| [Load/dict_ops](#loaddict_ops) | 33,098 | 0% | -77.60% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,397 | 0% | -56.95% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,303 | 0% | -78.02% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 424,668 | 0% | -53.13% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,796 | 0% | -73.84% | 27 | 0% | -88.16% | 3,720 | 0% | -67.41% |
| [Render/filters](#renderfilters) | 30,413 | 0% | -65.70% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,853 | 0% | -77.95% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,295 | 0% | -68.31% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,213 | 0% | -77.74% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,696 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,573 | 0% | -76.05% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,790 | 0% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,950 | 0% |  | 43 | 0% |  | 12,592 | 0% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,221,663 | 0% |  | 674 | 0% |  | 45,168 | 0% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,746 | 0% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,688 | 0% | -81.72% | 35 | 0% | -94.37% | 7,320 | 0% | +21.68% |
| [Load/int_arith](#loadint_arith) | 65,837 | 0% |  | 20 | 0% |  | 3,840 | 0% | -0.41% |
| [Render/int_arith](#renderint_arith) | 825,351 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,822 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,191 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,038 | 0% | -82.50% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,888 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,720 | 0% | -75.56% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 650,204 | 0% | -74.63% | 308 | 0% | -88.71% | 20,496 | 0% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,043,070 | 0% | -79.52% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 724,905 | 0% | -65.20% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,495 | 0% | -80.43% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,224,428 | 0% | -57.72% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,151,729 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,223,998 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,059 | 0% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,570,146 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,393,448 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,569,710 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
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
| Load/chat_llama | 736 | +0.82% | 734 | 0% | 0 |  | 7,853 | +3.68% |
| Render/chat_llama | 1,124 | +0.63% | 465 | -0.21% | 0 |  | 3,815 | +7.43% |
| Load/chat_mistral | 1,073 | +0.85% | 991 | +0.10% | 0 |  | 8,614 | +6.49% |
| Render/chat_mistral | 1,919 | +0.10% | 552 | 0% | 0 |  | 9,190 | -0.65% |
| Load/chat_qwen | 487 | +1.67% | 600 | 0% | 0 |  | 4,703 | +9.30% |
| Render/chat_qwen | 1,122 | +0.63% | 409 | +0.25% | 0 |  | 3,281 | +2.31% |
| Load/config_file | 184 | +6.36% | 158 | +0.64% | 0 |  | 3,109 | +5.50% |
| Render/config_file | 6,454 | +0.20% | 1,458 | +0.41% | 0 |  | 39,817 | -0.30% |
| Load/dict_ops | 31 | +3.33% | 16 | 0% | 0 |  | 1,314 | +1.47% |
| Render/dict_ops | 759 | 0% | 421 | 0% | 0 |  | 875 | +1.98% |
| Load/expressions | 41 | 0% | 24 | 0% | 0 |  | 1,289 | +3.62% |
| Render/expressions | 11 | 0% | 4 | 0% | 0 |  | 307 | -8.08% |
| Load/filters | 124 | +1.64% | 66 | +3.12% | 0 |  | 2,199 | +3.58% |
| Render/filters | 17 | 0% | 2 | 0% | 0 |  | 747 | -2.48% |
| Load/for_filter_if | 15 | 0% | 5 | 0% | 0 |  | 1,080 | +2.47% |
| Render/for_filter_if | 1,742 | +0.11% | 90 | +1.12% | 0 |  | 169 | -15.50% |
| Load/for_loop_vars | 25 | 0% | 17 | 0% | 0 |  | 1,085 | +2.17% |
| Render/for_loop_vars | 16 | +6.67% | 2 | +100.00% | 0 |  | 148 | -7.50% |
| Load/for_range | 5 | 0% | 5 | 0% | 0 |  | 754 | +4.87% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 116 | +22.11% |
| Load/html_autoescape | 477 | +1.92% | 513 | +0.20% | 0 |  | 4,218 | +6.81% |
| Render/html_autoescape | 4,642 | +0.06% | 1,671 | 0% | 0 |  | 34,795 | -0.81% |
| Load/inheritance | 11 | 0% | 4 | 0% | 0 |  | 685 | +2.39% |
| Render/inheritance | 617 | +0.65% | 104 | 0% | 0 |  | 893 | +1.25% |
| Load/int_arith | 102 | +0.99% | 96 | +1.05% | 0 |  | 2,048 | +4.01% |
| Render/int_arith | 17 | 0% | 3 | 0% | 0 |  | 756 | +0.13% |
| Module/keep | 6 | +200.00% | 2 | 0% | 0 |  | 362 | -0.82% |
| Module/keep_written | 2,934 | +0.07% | 557 | 0% | 0 |  | 678 | -0.44% |
| Load/large_static | 1,947 | 0% | 910 | 0% | 0 |  | 298 | +19.68% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 12 | +300.00% |
| RenderSink/large_static | 0 |  | 0 |  | 0 |  | 1 | 0% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 3 | 0% |
| Load/macros | 84 | -2.33% | 71 | -4.05% | 0 |  | 1,647 | +0.86% |
| Render/macros | 772 | +0.13% | 257 | 0% | 0 |  | 1,480 | -41.71% |
| Load/many_tags | 29,798 | +2.85% | 22,295 | +0.16% | 0 |  | 266,874 | +4.88% |
| Render/many_tags | 6,119 | 0% | 873 | 0% | 0 |  | 110 | -12.00% |
| Load/mitsuhiko_table | 39 | 0% | 53 | 0% | 0 |  | 848 | +0.36% |
| Render/mitsuhiko_table | 10,070 | +0.08% | 6,370 | +0.08% | 0 |  | 327 | -6.84% |
| RenderSink/mitsuhiko_table | 9,944 | +0.06% | 1,222 | +0.41% | 0 |  | 237 | -10.90% |
| RenderTo/mitsuhiko_table | 10,019 | +0.08% | 6,306 | +0.08% | 0 |  | 295 | -5.14% |
| Load/mitsuhiko_table_wide | 78 | 0% | 89 | 0% | 0 |  | 984 | -1.50% |
| Render/mitsuhiko_table_wide | 10,187 | +0.01% | 22,690 | 0% | 0 |  | 330 | -5.98% |
| RenderSink/mitsuhiko_table_wide | 10,016 | +0.03% | 1,287 | 0% | 0 |  | 247 | -4.63% |
| RenderTo/mitsuhiko_table_wide | 10,191 | +0.03% | 22,687 | 0% | 0 |  | 294 | -4.23% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | 0% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 155 | +2.65% | 88 | +1.15% | 0 |  | 2,858 | +2.69% |
| Render/strings | 446 | +0.68% | 479 | 0% | 0 |  | 1,483 | -1.07% |
| Load/substitute | 2 | 0% | 4 | 0% | 0 |  | 174 | +0.58% |
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
