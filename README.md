# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (168 so far, latest [d629ac1](https://github.com/jinja2cpp/Jinja2Cpp/commit/d629ac15afcae75ad9dd4d59278cdf50cba1951f) on 2026-11-01). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 343,666 | -0.12% |  | 55 | 0% |  | 18,368 | +0.70% | -53.00% |
| [Render/chat_llama](#renderchat_llama) | 310,839 | -0.20% |  | 251 | 0% |  | 11,696 | 0% | -18.55% |
| [Load/chat_mistral](#loadchat_mistral) | 392,630 | +0.16% |  | 52 | 0% |  | 20,792 | +0.54% | -53.43% |
| [Render/chat_mistral](#renderchat_mistral) | 516,839 | -0.06% |  | 424 | 0% |  | 10,064 | 0% | -23.80% |
| [Load/chat_qwen](#loadchat_qwen) | 266,500 | -0.08% |  | 44 | 0% |  | 14,432 | +0.22% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 296,070 | -0.14% |  | 238 | 0% |  | 8,304 | 0% | -2.35% |
| [Load/config_file](#loadconfig_file) | 96,930 | -1.66% |  | 25 | 0% |  | 5,464 | +0.59% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,269,667 | +0.03% |  | 908 | 0% |  | 24,264 | 0% | +1.92% |
| [Load/dict_ops](#loaddict_ops) | 33,199 | +0.32% | -77.53% | 9 | 0% | -93.13% | 2,176 | 0% | -48.87% |
| [Render/dict_ops](#renderdict_ops) | 299,200 | 0% | -56.69% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,250 | -0.01% | -78.05% | 16 | 0% | -89.04% | 2,728 | 0% | -55.54% |
| [Render/expressions](#renderexpressions) | 425,823 | 0% | -53.01% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 70,493 | -1.67% | -74.31% | 27 | 0% | -88.16% | 3,832 | +3.01% | -66.43% |
| [Render/filters](#renderfilters) | 30,389 | -0.11% | -65.73% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,786 | -0.25% | -78.00% | 7 | 0% | -94.31% | 1,904 | 0% | -44.78% |
| [Render/for_filter_if](#renderfor_filter_if) | 274,477 | 0% | -68.29% | 4 | 0% | -98.23% | 16,560 | 0% | +354.95% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,134 | -0.19% | -77.78% | 10 | 0% | -93.63% | 2,368 | 0% | -47.61% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,678 | 0% | -82.04% | 2 | 0% | -99.62% | 1,568 | 0% | -43.35% |
| [Load/for_range](#loadfor_range) | 13,571 | +0.03% | -76.06% | 7 | 0% | -87.50% | 1,088 | 0% | -45.60% |
| [Render/for_range](#renderfor_range) | 38,796 | 0% | -57.93% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 193,235 | -0.38% |  | 43 | 0% |  | 12,640 | +0.38% | -48.78% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,215,457 | -0.68% |  | 674 | 0% |  | 45,296 | 0% | +12.16% |
| [Load/inheritance](#loadinheritance) | 22,740 | 0% | -80.07% | 7 | 0% | -93.46% | 1,328 | 0% | -53.76% |
| [Render/inheritance](#renderinheritance) | 136,639 | 0% | -81.73% | 35 | 0% | -94.37% | 7,352 | 0% | +22.21% |
| [Load/int_arith](#loadint_arith) | 65,958 | +0.20% |  | 20 | 0% |  | 3,856 | +0.42% | 0% |
| [Render/int_arith](#renderint_arith) | 824,879 | -0.07% |  | 12 | 0% |  | 3,272 | 0% | -0.49% |
| [Module/keep](#modulekeep) | 6,893 | 0% |  | 7 | 0% |  |  |  |  |
| [Module/keep_written](#modulekeep_written) | 36,255 | 0% |  | 11 | 0% |  |  |  |  |
| [Load/large_static](#loadlarge_static) | 154,888 | 0% | -82.52% | 14 | 0% | -96.82% | 43,120 | 0% | -19.23% |
| [Render/large_static](#renderlarge_static) | 20,874 | 0% | -72.22% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,676 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,253 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,691 | -0.15% | -75.57% | 27 | 0% | -85.48% | 4,344 | 0% | -30.12% |
| [Render/macros](#rendermacros) | 652,277 | 0% | -74.55% | 308 | 0% | -88.71% | 20,528 | 0% | +39.46% |
| [Load/many_tags](#loadmany_tags) | 8,941,677 | -1.30% | -79.75% | 951 | 0% | -97.22% | 436,000 | +1.13% | -58.68% |
| [Render/many_tags](#rendermany_tags) | 725,492 | 0% | -65.17% | 2 | 0% | -90.48% | 5,648 | 0% | -10.52% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,427 | -0.05% | -80.46% | 19 | 0% | -89.67% | 3,360 | 0% | -44.59% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,222,409 | 0% | -57.73% | 33 | 0% | -99.19% | 381,536 | 0% | +9.95% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,149,546 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,221,815 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,003 | -0.04% |  | 35 | 0% |  | 5,688 | 0% | -31.17% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,568,181 | 0% |  | 33 | 0% |  | 1,414,928 | 0% | +2.46% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,391,484 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,567,746 | 0% |  | 32 | 0% |  | 37,064 | 0% | +28.59% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 672 | 0% | -40.00% |
| [Render/plain_text](#renderplain_text) | 1,007 | 0% | -85.18% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 707 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 808 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 71,362 | -0.65% | -75.55% | 30 | 0% | -86.61% | 4,328 | +1.88% | -52.12% |
| [Render/strings](#renderstrings) | 583,682 | +0.08% | -72.17% | 344 | 0% | -83.73% | 9,088 | 0% | -49.31% |
| [Load/substitute](#loadsubstitute) | 7,709 | 0% | -74.18% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,223 | 0% | -74.78% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,926 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,028 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 731 | +1.53% | 747 | +2.47% | 0 |  | 6,928 | -4.69% |
| Render/chat_llama | 1,188 | +2.41% | 465 | -0.21% | 0 |  | 3,848 | +0.97% |
| Load/chat_mistral | 1,111 | +1.83% | 1,040 | +2.46% | 0 |  | 7,832 | -3.55% |
| Render/chat_mistral | 1,942 | +0.05% | 563 | +2.36% | 0 |  | 9,834 | +5.92% |
| Load/chat_qwen | 497 | +0.20% | 605 | -0.98% | 0 |  | 4,228 | -4.06% |
| Render/chat_qwen | 1,189 | -1.57% | 392 | -1.75% | 0 |  | 3,231 | -1.67% |
| Load/config_file | 202 | +3.59% | 165 | -2.37% | 0 |  | 2,505 | -14.85% |
| Render/config_file | 6,558 | +8.67% | 1,421 | -4.44% | 0 |  | 40,427 | +1.89% |
| Load/dict_ops | 39 | +5.41% | 19 | 0% | 0 |  | 1,268 | -0.86% |
| Render/dict_ops | 759 | 0% | 410 | 0% | 0 |  | 847 | +1.07% |
| Load/expressions | 52 | 0% | 34 | +6.25% | 0 |  | 1,278 | -3.03% |
| Render/expressions | 9 | 0% | 3 | 0% | 0 |  | 356 | +14.10% |
| Load/filters | 140 | +2.19% | 81 | +24.62% | 0 |  | 1,442 | -30.30% |
| Render/filters | 10 | -16.67% | 1 | -50.00% | 0 |  | 787 | +7.81% |
| Load/for_filter_if | 13 | -27.78% | 6 | -40.00% | 0 |  | 1,008 | -1.27% |
| Render/for_filter_if | 1,734 | -0.52% | 90 | -5.26% | 0 |  | 193 | +30.41% |
| Load/for_loop_vars | 36 | +9.09% | 15 | +7.14% | 0 |  | 1,060 | -2.57% |
| Render/for_loop_vars | 13 | -38.10% | 0 | -100.00% | 0 |  | 210 | +100.00% |
| Load/for_range | 10 | +25.00% | 6 | 0% | 0 |  | 713 | -4.04% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 178 | +49.58% |
| Load/html_autoescape | 459 | +1.32% | 506 | -1.36% | 0 |  | 3,551 | -11.14% |
| Render/html_autoescape | 4,225 | +5.52% | 1,511 | +42.55% | 0 |  | 34,904 | +0.80% |
| Load/inheritance | 9 | -35.71% | 3 | 0% | 0 |  | 638 | -2.60% |
| Render/inheritance | 616 | +1.65% | 105 | 0% | 0 |  | 874 | -0.23% |
| Load/int_arith | 105 | +10.53% | 93 | +10.71% | 0 |  | 1,909 | -2.35% |
| Render/int_arith | 23 | +43.75% | 5 | +66.67% | 0 |  | 730 | +4.43% |
| Module/keep | 1 | 0% | 1 | 0% | 0 |  | 350 | +3.86% |
| Module/keep_written | 2,931 | +0.03% | 560 | +0.18% | 0 |  | 660 | -0.30% |
| Load/large_static | 1,948 | 0% | 914 | +0.22% | 0 |  | 249 | +2.05% |
| Render/large_static | 709 | 0% | 570 | 0% | 0 |  | 3 | -40.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 1 | -50.00% |
| RenderTo/large_static | 687 | 0% | 570 | 0% | 0 |  | 3 | 0% |
| Load/macros | 71 | +2.90% | 65 | -2.99% | 0 |  | 1,624 | -1.69% |
| Render/macros | 762 | -0.26% | 262 | +0.77% | 0 |  | 725 | -51.37% |
| Load/many_tags | 29,530 | +0.73% | 22,694 | +1.38% | 0 |  | 218,375 | -15.04% |
| Render/many_tags | 6,188 | +0.91% | 875 | +1.63% | 0 |  | 174 | +6.75% |
| Load/mitsuhiko_table | 43 | +48.28% | 55 | +7.84% | 0 |  | 835 | +5.70% |
| Render/mitsuhiko_table | 10,075 | -0.12% | 6,365 | -0.19% | 0 |  | 376 | +11.57% |
| RenderSink/mitsuhiko_table | 9,963 | +0.01% | 1,224 | -0.57% | 0 |  | 298 | +8.76% |
| RenderTo/mitsuhiko_table | 10,039 | 0% | 6,301 | -0.10% | 0 |  | 336 | +10.53% |
| Load/mitsuhiko_table_wide | 83 | +16.90% | 91 | +9.64% | 0 |  | 1,014 | +1.81% |
| Render/mitsuhiko_table_wide | 10,212 | -0.13% | 22,685 | -0.03% | 0 |  | 406 | +24.92% |
| RenderSink/mitsuhiko_table_wide | 10,036 | -0.09% | 1,293 | -0.39% | 0 |  | 332 | +21.61% |
| RenderTo/mitsuhiko_table_wide | 10,212 | -0.25% | 22,690 | +0.01% | 0 |  | 371 | +20.06% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | -6.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| Load/strings | 167 | +8.44% | 97 | +7.78% | 0 |  | 2,614 | -6.17% |
| Render/strings | 451 | +2.04% | 465 | -0.43% | 0 |  | 1,544 | +2.32% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 145 | -12.12% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 1 | -50.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | 0% |

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
