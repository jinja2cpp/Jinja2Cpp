# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (155 so far, latest [3929ac7](https://github.com/jinja2cpp/Jinja2Cpp/commit/3929ac75823552b8aa8c8063f72ed4616601cb14) on 2026-10-29). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,978 | 0% |  | 55 | 0% |  | 18,256 | 0% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 309,841 | 0% |  | 251 | 0% |  | 11,712 | 0% | -18.44% |
| [Load/chat_mistral](#loadchat_mistral) | 393,317 | 0% |  | 52 | 0% |  | 20,696 | 0% | -53.65% |
| [Render/chat_mistral](#renderchat_mistral) | 513,966 | 0% |  | 424 | 0% |  | 10,160 | 0% | -23.08% |
| [Load/chat_qwen](#loadchat_qwen) | 267,674 | 0% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 294,989 | 0% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 99,274 | 0% |  | 25 | 0% |  | 5,448 | 0% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,263,049 | 0% |  | 908 | 0% |  | 24,008 | 0% | +0.84% |
| [Load/dict_ops](#loaddict_ops) | 33,153 | 0% | -77.57% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,629 | 0% | -57.06% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,320 | 0% | -78.02% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 424,641 | 0% | -53.14% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,756 | 0% | -73.85% | 27 | 0% | -88.16% | 3,736 | 0% | -67.27% |
| [Render/filters](#renderfilters) | 30,319 | 0% | -65.81% | 26 | 0% | -64.86% | 1,096 | 0% | -77.69% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,065 | 0% | -77.79% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,093 | 0% | -68.33% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,478 | 0% | -77.59% | 10 | 0% | -93.63% | 2,384 | 0% | -47.26% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,694 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,573 | 0% | -76.05% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,763 | 0% | -57.96% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,427 | 0% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,218,722 | 0% |  | 674 | 0% |  | 45,168 | 0% | +11.85% |
| [Load/inheritance](#loadinheritance) | 22,746 | 0% | -80.07% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,617 | 0% | -81.73% | 35 | 0% | -94.37% | 7,320 | 0% | +21.68% |
| [Load/int_arith](#loadint_arith) | 66,087 | 0% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 824,528 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Module/keep](#modulekeep) | 6,851 | 0% |  | 8 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,225 | 0% |  | 12 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 155,038 | 0% | -82.50% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,888 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,690 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,267 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,853 | 0% | -75.50% | 27 | 0% | -85.48% | 4,376 | 0% | -29.60% |
| [Render/macros](#rendermacros) | 650,102 | 0% | -74.64% | 308 | 0% | -88.71% | 20,496 | 0% | +39.24% |
| [Load/many_tags](#loadmany_tags) | 9,088,453 | 0% | -79.42% | 951 | 0% | -97.22% | 431,120 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 723,405 | 0% | -65.27% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,495 | 0% | -80.43% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,222,903 | 0% | -57.73% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,150,153 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,222,428 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,059 | -0.01% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,568,867 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,392,172 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,568,434 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,021 | 0% | -84.97% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 721 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 822 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,971 | 0% | -75.34% | 30 | 0% | -86.61% | 4,264 | 0% | -52.83% |
| [Render/strings](#renderstrings) | 581,137 | 0% | -72.29% | 344 | 0% | -83.73% | 9,120 | 0% | -49.13% |
| [Load/substitute](#loadsubstitute) | 7,715 | 0% | -74.16% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,237 | 0% | -74.62% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,940 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,042 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 724 | 0% | 724 | 0% | 0 |  | 7,206 | -8.51% |
| Render/chat_llama | 1,122 | +0.09% | 478 | 0% | 0 |  | 3,498 | -4.76% |
| Load/chat_mistral | 1,124 | 0% | 1,000 | 0% | 0 |  | 7,975 | -7.80% |
| Render/chat_mistral | 1,852 | +0.16% | 543 | +0.37% | 0 |  | 10,000 | -0.16% |
| Load/chat_qwen | 505 | 0% | 604 | 0% | 0 |  | 4,131 | -9.47% |
| Render/chat_qwen | 1,083 | +0.09% | 386 | 0% | 0 |  | 3,234 | +0.31% |
| Load/config_file | 181 | +1.69% | 166 | +0.61% | 0 |  | 2,929 | -0.51% |
| Render/config_file | 5,790 | 0% | 1,438 | 0% | 0 |  | 38,227 | -0.56% |
| Load/dict_ops | 34 | +3.03% | 20 | +11.11% | 0 |  | 1,282 | -1.16% |
| Render/dict_ops | 772 | 0% | 418 | 0% | 0 |  | 851 | -1.39% |
| Load/expressions | 57 | +3.64% | 34 | +3.03% | 0 |  | 1,293 | -6.10% |
| Render/expressions | 4 | 0% | 2 | 0% | 0 |  | 297 | +6.83% |
| Load/filters | 135 | +0.75% | 70 | 0% | 0 |  | 1,965 | -7.14% |
| Render/filters | 25 | 0% | 6 | 0% | 0 |  | 733 | -3.55% |
| Load/for_filter_if | 7 | 0% | 5 | 0% | 0 |  | 1,029 | -1.63% |
| Render/for_filter_if | 1,730 | 0% | 88 | 0% | 0 |  | 153 | -25.73% |
| Load/for_loop_vars | 30 | +11.11% | 13 | +8.33% | 0 |  | 1,020 | -2.49% |
| Render/for_loop_vars | 13 | 0% | 3 | 0% | 0 |  | 153 | -8.93% |
| Load/for_range | 4 | 0% | 3 | 0% | 0 |  | 724 | -0.55% |
| Render/for_range | 4 | 0% | 2 | 0% | 0 |  | 109 | +11.22% |
| Load/html_autoescape | 454 | 0% | 502 | 0% | 0 |  | 3,736 | -4.23% |
| Render/html_autoescape | 4,526 | 0% | 1,384 | +0.14% | 0 |  | 32,612 | -8.23% |
| Load/inheritance | 4 | 0% | 3 | 0% | 0 |  | 651 | +0.93% |
| Render/inheritance | 596 | 0% | 96 | 0% | 0 |  | 897 | +3.70% |
| Load/int_arith | 124 | -0.80% | 94 | -3.09% | 0 |  | 1,957 | -8.08% |
| Render/int_arith | 15 | 0% | 4 | 0% | 0 |  | 722 | +3.44% |
| Module/keep | 3 | 0% | 1 | 0% | 0 |  | 312 | +0.97% |
| Module/keep_written | 2,923 | 0% | 554 | 0% | 0 |  | 644 | +0.78% |
| Load/large_static | 1,942 | 0% | 911 | 0% | 0 |  | 230 | -15.13% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 5 | -87.80% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | -50.00% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 3 | -80.00% |
| Load/macros | 81 | +1.25% | 72 | +1.41% | 0 |  | 1,638 | +0.49% |
| Render/macros | 766 | 0% | 260 | 0% | 0 |  | 1,192 | +34.08% |
| Load/many_tags | 30,678 | 0% | 22,369 | 0% | 0 |  | 240,141 | -5.94% |
| Render/many_tags | 6,117 | 0% | 865 | 0% | 0 |  | 155 | -40.15% |
| Load/mitsuhiko_table | 44 | +10.00% | 56 | +1.82% | 0 |  | 812 | -2.05% |
| Render/mitsuhiko_table | 10,040 | 0% | 6,362 | 0% | 0 |  | 338 | +7.30% |
| RenderSink/mitsuhiko_table | 9,921 | 0% | 1,208 | 0% | 0 |  | 263 | -1.87% |
| RenderTo/mitsuhiko_table | 10,011 | 0% | 6,293 | 0% | 0 |  | 301 | +3.79% |
| Load/mitsuhiko_table_wide | 76 | -9.52% | 96 | -3.03% | 0 |  | 945 | +1.39% |
| Render/mitsuhiko_table_wide | 10,219 | 0% | 22,687 | 0% | 0 |  | 309 | 0% |
| RenderSink/mitsuhiko_table_wide | 10,044 | 0% | 1,287 | 0% | 0 |  | 218 | -6.44% |
| RenderTo/mitsuhiko_table_wide | 10,221 | 0% | 22,676 | 0% | 0 |  | 275 | -4.51% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -73.91% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 8 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | -16.67% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 7 | 0% |
| Load/strings | 172 | 0% | 98 | -1.01% | 0 |  | 2,727 | -2.88% |
| Render/strings | 442 | 0% | 473 | 0% | 0 |  | 2,330 | +54.41% |
| Load/substitute | 2 | 0% | 4 | +33.33% | 0 |  | 92 | -36.99% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | -50.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 1 | -75.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | -60.00% |

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
