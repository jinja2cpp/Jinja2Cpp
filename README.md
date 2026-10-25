# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (138 so far, latest [3986280](https://github.com/jinja2cpp/Jinja2Cpp/commit/39862807e9fcc9cc71d403761192672d3493bc4d) on 2026-10-25). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,446 | 0% |  | 55 | 0% |  | 18,256 | 0% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 306,165 | 0% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 393,225 | 0% |  | 52 | 0% |  | 20,728 | 0% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 508,113 | +0.01% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 265,906 | 0% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 293,515 | -0.01% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,832 | 0% |  | 25 | 0% |  | 5,448 | 0% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,259,027 | +0.05% |  | 908 | 0% |  | 23,400 | 0% | -1.71% |
| [Load/dict_ops](#loaddict_ops) | 33,169 | +0.02% | -77.55% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 297,504 | +0.11% | -56.94% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,364 | 0% | -77.99% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 423,480 | 0% | -53.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,813 | -0.01% | -73.83% | 27 | 0% | -88.16% | 3,736 | 0% | -67.27% |
| [Render/filters](#renderfilters) | 30,228 | +0.02% | -65.91% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,880 | 0% | -77.93% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,232 | 0% | -68.43% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,379 | 0% | -77.65% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,340 | 0% | -82.07% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,570 | 0% | -76.06% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,751 | 0% | -57.98% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,757 | 0% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,824 | 0% |  | 674 | 0% |  | 43,728 | 0% | +8.28% |
| [Load/inheritance](#loadinheritance) | 22,745 | 0% | -80.07% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,508 | +0.01% | -81.75% | 35 | 0% | -94.37% | 6,472 | 0% | +7.58% |
| [Load/int_arith](#loadint_arith) | 66,056 | 0% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 823,350 | +0.13% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,234 | 0% | -82.48% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,881 | 0% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,683 | +0.01% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,465 | 0% | -75.67% | 27 | 0% | -85.48% | 4,376 | 0% | -29.60% |
| [Render/macros](#rendermacros) | 650,733 | 0% | -74.61% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,080,951 | 0% | -79.44% | 951 | 0% | -97.22% | 431,152 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,442 | 0% | -65.37% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,341 | 0% | -80.49% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,189,138 | 0% | -58.00% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,116,411 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,188,688 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,000 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,535,179 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,358,490 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,534,748 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,689 | 0% | -61.87% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,014 | +0.10% | -85.07% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 714 | +0.11% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 815 | +0.10% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,263 | -0.02% | -75.24% | 30 | 0% | -86.61% | 4,264 | 0% | -52.83% |
| [Render/strings](#renderstrings) | 577,539 | 0% | -72.47% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,712 | 0% | -74.17% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,230 | +0.04% | -74.70% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,933 | +0.04% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | +0.04% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 699 | +1.75% | 724 | +0.56% | 0 |  | 7,507 | +0.43% |
| Render/chat_llama | 1,139 | -1.56% | 485 | -1.22% | 0 |  | 3,586 | -1.78% |
| Load/chat_mistral | 1,068 | +0.75% | 1,008 | -0.20% | 0 |  | 8,491 | +0.44% |
| Render/chat_mistral | 1,848 | +3.36% | 556 | +2.77% | 0 |  | 8,994 | -3.96% |
| Load/chat_qwen | 474 | +3.04% | 593 | -0.17% | 0 |  | 4,474 | +2.19% |
| Render/chat_qwen | 1,045 | -0.76% | 392 | +2.35% | 0 |  | 3,086 | +0.03% |
| Load/config_file | 172 | -5.49% | 157 | -2.48% | 0 |  | 3,033 | +1.51% |
| Render/config_file | 5,870 | +3.36% | 1,449 | +6.62% | 0 |  | 38,693 | +3.15% |
| Load/dict_ops | 32 | +14.29% | 17 | 0% | 0 |  | 1,343 | +0.60% |
| Render/dict_ops | 733 | +0.41% | 417 | -0.71% | 0 |  | 830 | +0.61% |
| Load/expressions | 27 | +22.73% | 14 | +40.00% | 0 |  | 1,297 | +1.33% |
| Render/expressions | 4 | 0% | 1 | 0% | 0 |  | 321 | +0.94% |
| Load/filters | 136 | +4.62% | 71 | +1.43% | 0 |  | 2,167 | -1.19% |
| Render/filters | 27 | +92.86% | 6 | 0% | 0 |  | 734 | +0.82% |
| Load/for_filter_if | 9 | -25.00% | 4 | 0% | 0 |  | 1,040 | +1.96% |
| Render/for_filter_if | 1,653 | +0.18% | 62 | -4.62% | 0 |  | 187 | +6.25% |
| Load/for_loop_vars | 22 | +15.79% | 8 | -27.27% | 0 |  | 1,079 | +2.66% |
| Render/for_loop_vars | 7 | +75.00% | 2 | +100.00% | 0 |  | 138 | -9.80% |
| Load/for_range | 3 | +50.00% | 3 | +50.00% | 0 |  | 722 | -1.23% |
| Render/for_range | 1 | 0% | 1 |  | 0 |  | 103 | -9.65% |
| Load/html_autoescape | 471 | -1.05% | 527 | -0.57% | 0 |  | 3,950 | -1.08% |
| Render/html_autoescape | 4,201 | -4.13% | 1,235 | -1.59% | 0 |  | 33,874 | +0.63% |
| Load/inheritance | 3 | +50.00% | 2 | 0% | 0 |  | 659 | +5.95% |
| Render/inheritance | 570 | +0.35% | 89 | -5.32% | 0 |  | 854 | +3.14% |
| Load/int_arith | 83 | +7.79% | 51 | +6.25% | 0 |  | 2,038 | +0.59% |
| Render/int_arith | 16 | +77.78% | 4 | +33.33% | 0 |  | 657 | -7.59% |
| Load/large_static | 1,942 | -0.10% | 909 | -0.11% | 0 |  | 293 | -8.44% |
| Render/large_static | 706 | +0.14% | 568 | -0.35% | 0 |  | 10 | +233.33% |
| RenderSink/large_static | 0 |  | 1 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 687 | +0.44% | 568 | -0.53% | 0 |  | 4 | +100.00% |
| Load/macros | 62 | 0% | 40 | 0% | 0 |  | 1,665 | +2.15% |
| Render/macros | 734 | +1.10% | 217 | +1.40% | 0 |  | 541 | -3.74% |
| Load/many_tags | 27,700 | -0.05% | 22,246 | 0% | 0 |  | 251,267 | -2.23% |
| Render/many_tags | 6,121 | -0.02% | 865 | -0.57% | 0 |  | 175 | +65.09% |
| Load/mitsuhiko_table | 19 | +26.67% | 20 | 0% | 0 |  | 838 | +2.95% |
| Render/mitsuhiko_table | 9,044 | -0.30% | 6,618 | -0.15% | 0 |  | 272 | -6.53% |
| RenderSink/mitsuhiko_table | 9,003 | -0.10% | 1,499 | +0.40% | 0 |  | 204 | -16.05% |
| RenderTo/mitsuhiko_table | 9,057 | -0.34% | 6,629 | -0.23% | 0 |  | 246 | -4.65% |
| Load/mitsuhiko_table_wide | 85 | -2.30% | 68 | -1.45% | 0 |  | 953 | -7.74% |
| Render/mitsuhiko_table_wide | 9,403 | -9.04% | 23,080 | -0.43% | 0 |  | 287 | -6.51% |
| RenderSink/mitsuhiko_table_wide | 9,117 | -0.31% | 1,585 | -1.49% | 0 |  | 207 | -16.19% |
| RenderTo/mitsuhiko_table_wide | 9,395 | -9.08% | 23,025 | -0.46% | 0 |  | 261 | 0% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -62.50% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | +40.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | +33.33% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 7 | +75.00% |
| Load/strings | 172 | +0.58% | 112 | -0.88% | 0 |  | 2,884 | -0.21% |
| Render/strings | 434 | +7.96% | 457 | +0.44% | 0 |  | 2,330 | +54.92% |
| Load/substitute | 2 | 0% | 2 | 0% | 0 |  | 177 | -20.98% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
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
