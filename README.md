# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (169 so far, latest [808cbdb](https://github.com/jinja2cpp/Jinja2Cpp/commit/808cbdb9257d6944670cc1e2a08f7015784c8888) on 2026-11-02). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,813 | +0.04% |  | 55 | 0% |  | 18,368 | 0% | -53.00% |
| [Render/chat_llama](#renderchat_llama) | 294,756 | -5.17% |  | 225 | -10.36% |  | 11,616 | -0.68% | -19.11% |
| [Load/chat_mistral](#loadchat_mistral) | 392,674 | +0.01% |  | 52 | 0% |  | 20,792 | 0% | -53.43% |
| [Render/chat_mistral](#renderchat_mistral) | 500,216 | -3.22% |  | 402 | -5.19% |  | 9,984 | -0.79% | -24.41% |
| [Load/chat_qwen](#loadchat_qwen) | 266,433 | -0.03% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 279,840 | -5.48% |  | 212 | -10.92% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,896 | -0.04% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,270,659 | +0.08% |  | 908 | 0% |  | 24,264 | 0% | +1.92% |
| [Load/dict_ops](#loaddict_ops) | 33,181 | -0.05% | -77.55% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 297,406 | -0.60% | -56.95% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,247 | -0.01% | -78.05% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 427,426 | +0.38% | -52.83% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 70,499 | +0.01% | -74.31% | 27 | 0% | -88.16% | 3,832 | 0% | -66.43% |
| [Render/filters](#renderfilters) | 30,225 | -0.54% | -65.91% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,775 | -0.04% | -78.01% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,620 | +0.05% | -68.27% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,128 | -0.01% | -77.79% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,681 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,568 | -0.02% | -76.06% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,799 | +0.01% | -57.92% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,184 | -0.03% |  | 43 | 0% |  | 12,640 | 0% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,207,429 | -0.66% |  | 674 | 0% |  | 45,296 | 0% | +12.16% |
| [Load/inheritance](#loadinheritance) | 22,737 | -0.01% | -80.08% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,309 | -0.24% | -81.77% | 35 | 0% | -94.37% | 7,352 | 0% | +22.21% |
| [Load/int_arith](#loadint_arith) | 65,964 | +0.01% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 824,915 | 0% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,893 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,261 | +0.02% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,888 | 0% | -82.52% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,874 | 0% | -72.22% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,676 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,253 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,654 | -0.07% | -75.59% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 652,280 | 0% | -74.55% | 308 | 0% | -88.71% | 20,528 | 0% | +39.46% |
| [Load/many_tags](#loadmany_tags) | 8,939,577 | -0.02% | -79.76% | 951 | 0% | -97.22% | 436,000 | 0% | -58.68% |
| [Render/many_tags](#rendermany_tags) | 725,492 | 0% | -65.17% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,414 | -0.03% | -80.46% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,225,415 | +0.06% | -57.71% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,152,552 | +0.06% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,224,821 | +0.06% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,990 | -0.02% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,571,187 | +0.05% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,394,490 | +0.06% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,570,752 | +0.05% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,007 | 0% | -85.18% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 707 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 808 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,331 | -0.04% | -75.56% | 30 | 0% | -86.61% | 4,328 | 0% | -52.12% |
| [Render/strings](#renderstrings) | 583,688 | 0% | -72.17% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,223 | 0% | -74.78% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,926 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,028 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 722 | -1.23% | 746 | -0.13% | 0 |  | 7,480 | +7.97% |
| Render/chat_llama | 1,158 | -2.53% | 465 | 0% | 0 |  | 3,550 | -7.74% |
| Load/chat_mistral | 1,108 | -0.27% | 1,040 | 0% | 0 |  | 8,706 | +11.16% |
| Render/chat_mistral | 1,883 | -3.04% | 547 | -2.84% | 0 |  | 9,613 | -2.25% |
| Load/chat_qwen | 480 | -3.42% | 604 | -0.17% | 0 |  | 4,545 | +7.50% |
| Render/chat_qwen | 1,124 | -5.47% | 395 | +0.77% | 0 |  | 2,974 | -7.95% |
| Load/config_file | 182 | -9.90% | 166 | +0.61% | 0 |  | 2,696 | +7.62% |
| Render/config_file | 5,943 | -9.38% | 1,452 | +2.18% | 0 |  | 41,331 | +2.24% |
| Load/dict_ops | 37 | -5.13% | 17 | -10.53% | 0 |  | 1,292 | +1.89% |
| Render/dict_ops | 764 | +0.66% | 417 | +1.71% | 0 |  | 877 | +3.54% |
| Load/expressions | 62 | +19.23% | 40 | +17.65% | 0 |  | 1,328 | +3.91% |
| Render/expressions | 7 | -22.22% | 2 | -33.33% | 0 |  | 310 | -12.92% |
| Load/filters | 132 | -5.71% | 81 | 0% | 0 |  | 1,679 | +16.44% |
| Render/filters | 16 | +60.00% | 4 | +300.00% | 0 |  | 760 | -3.43% |
| Load/for_filter_if | 13 | 0% | 7 | +16.67% | 0 |  | 1,011 | +0.30% |
| Render/for_filter_if | 1,742 | +0.46% | 94 | +4.44% | 0 |  | 183 | -5.18% |
| Load/for_loop_vars | 34 | -5.56% | 17 | +13.33% | 0 |  | 1,105 | +4.25% |
| Render/for_loop_vars | 9 | -30.77% | 1 |  | 0 |  | 176 | -16.19% |
| Load/for_range | 14 | +40.00% | 9 | +50.00% | 0 |  | 689 | -3.37% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 104 | -41.57% |
| Load/html_autoescape | 446 | -2.83% | 503 | -0.59% | 0 |  | 3,904 | +9.94% |
| Render/html_autoescape | 4,925 | +16.57% | 1,496 | -0.99% | 0 |  | 36,551 | +4.72% |
| Load/inheritance | 11 | +22.22% | 3 | 0% | 0 |  | 639 | +0.16% |
| Render/inheritance | 609 | -1.14% | 104 | -0.95% | 0 |  | 851 | -2.63% |
| Load/int_arith | 111 | +5.71% | 92 | -1.08% | 0 |  | 1,974 | +3.40% |
| Render/int_arith | 18 | -21.74% | 2 | -60.00% | 0 |  | 764 | +4.66% |
| Module/keep | 6 | +500.00% | 2 | +100.00% | 0 |  | 367 | +4.86% |
| Module/keep_written | 2,934 | +0.10% | 560 | 0% | 0 |  | 692 | +4.85% |
| Load/large_static | 1,944 | -0.21% | 914 | 0% | 0 |  | 294 | +18.07% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 4 | +33.33% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | +100.00% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 88 | +23.94% | 78 | +20.00% | 0 |  | 1,709 | +5.23% |
| Render/macros | 758 | -0.52% | 263 | +0.38% | 0 |  | 2,496 | +244.28% |
| Load/many_tags | 28,356 | -3.98% | 22,652 | -0.19% | 0 |  | 223,006 | +2.12% |
| Render/many_tags | 6,184 | -0.06% | 872 | -0.34% | 0 |  | 165 | -5.17% |
| Load/mitsuhiko_table | 46 | +6.98% | 60 | +9.09% | 0 |  | 835 | 0% |
| Render/mitsuhiko_table | 10,071 | -0.04% | 6,364 | -0.02% | 0 |  | 358 | -4.79% |
| RenderSink/mitsuhiko_table | 9,953 | -0.10% | 1,216 | -0.65% | 0 |  | 288 | -3.36% |
| RenderTo/mitsuhiko_table | 10,020 | -0.19% | 6,300 | -0.02% | 0 |  | 331 | -1.49% |
| Load/mitsuhiko_table_wide | 79 | -4.82% | 91 | 0% | 0 |  | 958 | -5.52% |
| Render/mitsuhiko_table_wide | 10,211 | -0.01% | 22,685 | 0% | 0 |  | 367 | -9.61% |
| RenderSink/mitsuhiko_table_wide | 10,028 | -0.08% | 1,286 | -0.54% | 0 |  | 294 | -11.45% |
| RenderTo/mitsuhiko_table_wide | 10,200 | -0.12% | 22,685 | -0.02% | 0 |  | 334 | -9.97% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -57.14% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | -20.00% |
| Load/strings | 163 | -2.40% | 98 | +1.03% | 0 |  | 2,716 | +3.90% |
| Render/strings | 451 | 0% | 467 | +0.43% | 0 |  | 1,535 | -0.58% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 151 | +4.14% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 2 | +100.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | +100.00% |

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
