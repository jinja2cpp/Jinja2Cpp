# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (131 so far, latest [6cee627](https://github.com/jinja2cpp/Jinja2Cpp/commit/6cee6275bc44a502ffedbf54f5107d0d97f7bad2) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 346,691 | +0.33% |  | 55 | 0% |  | 18,304 | 0% | -53.16% |
| [Render/chat_llama](#renderchat_llama) | 306,023 | +0.02% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 395,649 | +0.16% |  | 52 | +1.96% |  | 20,776 | 0% | -53.47% |
| [Render/chat_mistral](#renderchat_mistral) | 508,446 | +0.03% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 268,617 | +0.65% |  | 44 | 0% |  | 14,448 | 0% | -47.13% |
| [Render/chat_qwen](#renderchat_qwen) | 293,530 | 0% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,218 | +0.16% |  | 25 | 0% |  | 5,496 | 0% | -47.95% |
| [Render/config_file](#renderconfig_file) | 1,257,747 | +0.06% |  | 908 | 0% |  | 23,384 | 0% | -1.78% |
| [Load/dict_ops](#loaddict_ops) | 33,327 | +0.16% | -77.45% | 9 | 0% | -93.13% | 2,240 | 0% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 296,678 | -0.01% | -57.06% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,509 | +0.05% | -77.92% | 16 | 0% | -89.04% | 2,792 | 0% | -54.50% |
| [Render/expressions](#renderexpressions) | 423,474 | 0% | -53.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,946 | -0.02% | -73.78% | 27 | 0% | -88.16% | 3,784 | 0% | -66.85% |
| [Render/filters](#renderfilters) | 30,217 | +0.14% | -65.92% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,103 | +0.15% | -77.77% | 7 | 0% | -94.31% | 1,968 | 0% | -42.92% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,120 | 0% | -68.56% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,618 | +0.14% | -77.52% | 10 | 0% | -93.63% | 2,448 | 0% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,738 | 0% | -81.85% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,679 | +0.05% | -75.87% | 7 | 0% | -87.50% | 1,152 | 0% | -42.40% |
| [Render/for_range](#renderfor_range) | 38,745 | -0.04% | -57.98% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,143 | +0.17% |  | 43 | 0% |  | 12,656 | 0% | -48.72% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,212,058 | +0.03% |  | 674 | 0% |  | 43,712 | 0% | +8.24% |
| [Load/inheritance](#loadinheritance) | 23,164 | +0.01% | -79.70% | 7 | 0% | -93.46% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 136,580 | 0% | -81.74% | 35 | 0% | -94.37% | 6,472 | 0% | +7.58% |
| [Load/int_arith](#loadint_arith) | 66,311 | +0.18% |  | 20 | 0% |  | 3,904 | 0% | +1.24% |
| [Render/int_arith](#renderint_arith) | 821,268 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 154,961 | -0.03% | -82.51% | 14 | 0% | -96.82% | 43,184 | 0% | -19.11% |
| [Render/large_static](#renderlarge_static) | 20,881 | 0% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,691 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,524 | +0.03% | -75.64% | 27 | 0% | -85.48% | 4,424 | 0% | -28.83% |
| [Render/macros](#rendermacros) | 649,130 | 0% | -74.67% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,125,189 | +0.05% | -79.34% | 951 | 0% | -97.22% | 431,184 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 722,334 | +0.08% | -65.32% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,375 | +0.03% | -80.48% | 19 | 0% | -89.67% | 3,424 | 0% | -43.54% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,129 | 0% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,410 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,679 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,711 | +0.03% |  | 35 | 0% |  | 5,752 | 0% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,175 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,478 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,712 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,747 | 0% | -61.27% | 4 | 0% | -77.78% | 736 | 0% | -34.29% |
| [Render/plain_text](#renderplain_text) | 1,013 | 0% | -85.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 719 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,513 | +0.28% | -75.15% | 30 | +3.45% | -86.61% | 4,312 | 0% | -52.30% |
| [Render/strings](#renderstrings) | 577,938 | +0.08% | -72.45% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,774 | -0.03% | -73.96% | 6 | 0% | -83.78% | 896 | 0% | -41.05% |
| [Render/substitute](#rendersubstitute) | 2,230 | 0% | -74.70% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,941 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 730 | +2.53% | 724 | +1.54% | 0 |  | 7,862 | +3.58% |
| Render/chat_llama | 1,124 | -1.92% | 495 | +1.02% | 0 |  | 3,551 | -0.84% |
| Load/chat_mistral | 1,057 | +1.93% | 1,002 | -0.10% | 0 |  | 9,085 | +5.28% |
| Render/chat_mistral | 1,862 | +0.87% | 527 | 0% | 0 |  | 9,339 | -1.48% |
| Load/chat_qwen | 467 | +0.43% | 607 | +0.66% | 0 |  | 4,992 | +9.38% |
| Render/chat_qwen | 1,070 | +2.59% | 397 | +1.79% | 0 |  | 3,139 | -6.44% |
| Load/config_file | 192 | +2.13% | 169 | -0.59% | 0 |  | 3,177 | +3.76% |
| Render/config_file | 5,816 | -6.24% | 1,938 | +2.98% | 0 |  | 38,726 | -3.98% |
| Load/dict_ops | 40 | +14.29% | 22 | +37.50% | 0 |  | 1,325 | 0% |
| Render/dict_ops | 695 | -1.84% | 427 | +1.18% | 0 |  | 849 | -0.70% |
| Load/expressions | 41 | +78.26% | 23 | +130.00% | 0 |  | 1,407 | +2.10% |
| Render/expressions | 3 | -50.00% | 1 | -50.00% | 0 |  | 276 | -2.13% |
| Load/filters | 140 | +12.00% | 89 | +18.67% | 0 |  | 2,118 | -13.09% |
| Render/filters | 18 | -5.26% | 5 | 0% | 0 |  | 720 | -2.44% |
| Load/for_filter_if | 12 | 0% | 5 | +66.67% | 0 |  | 1,161 | +7.10% |
| Render/for_filter_if | 1,649 | +0.06% | 63 | -8.70% | 0 |  | 213 | +27.54% |
| Load/for_loop_vars | 28 | +75.00% | 9 | +80.00% | 0 |  | 1,190 | +8.28% |
| Render/for_loop_vars | 6 | 0% | 3 | 0% | 0 |  | 186 | +16.98% |
| Load/for_range | 3 | 0% | 3 | 0% | 0 |  | 752 | +0.67% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 167 | +46.49% |
| Load/html_autoescape | 483 | +1.47% | 517 | -0.39% | 0 |  | 4,810 | +14.66% |
| Render/html_autoescape | 4,424 | +5.89% | 1,464 | +4.80% | 0 |  | 34,198 | +7.33% |
| Load/inheritance | 3 | 0% | 2 | 0% | 0 |  | 632 | -4.53% |
| Render/inheritance | 589 | -0.67% | 120 | 0% | 0 |  | 816 | -4.00% |
| Load/int_arith | 83 | +7.79% | 57 | +23.91% | 0 |  | 2,327 | +9.66% |
| Render/int_arith | 16 | +45.45% | 8 | +60.00% | 0 |  | 670 | -6.42% |
| Load/large_static | 1,939 | -0.36% | 909 | +0.11% | 0 |  | 243 | +3.85% |
| Render/large_static | 707 | 0% | 568 | 0% | 0 |  | 5 | +25.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 686 | 0% | 568 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 68 | -1.45% | 59 | +20.41% | 0 |  | 1,690 | +3.36% |
| Render/macros | 726 | +0.14% | 212 | 0% | 0 |  | 532 | -73.69% |
| Load/many_tags | 27,754 | -0.33% | 22,278 | +0.08% | 0 |  | 268,095 | +2.78% |
| Render/many_tags | 6,119 | -0.13% | 853 | +0.71% | 0 |  | 160 | +7.38% |
| Load/mitsuhiko_table | 22 | +10.00% | 21 | -4.55% | 0 |  | 821 | -1.79% |
| Render/mitsuhiko_table | 9,032 | -0.21% | 6,614 | +0.05% | 0 |  | 347 | +14.14% |
| RenderSink/mitsuhiko_table | 8,995 | -0.14% | 1,491 | +0.13% | 0 |  | 287 | +12.55% |
| RenderTo/mitsuhiko_table | 9,030 | -0.21% | 6,622 | +0.12% | 0 |  | 327 | +18.48% |
| Load/mitsuhiko_table_wide | 93 | -4.12% | 95 | +1.06% | 0 |  | 1,029 | +6.30% |
| Render/mitsuhiko_table_wide | 9,217 | +0.28% | 23,044 | -0.02% | 0 |  | 327 | +6.17% |
| RenderSink/mitsuhiko_table_wide | 9,108 | +0.10% | 1,532 | -0.39% | 0 |  | 269 | +8.03% |
| RenderTo/mitsuhiko_table_wide | 9,206 | +0.28% | 22,993 | -0.02% | 0 |  | 318 | +7.80% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 35 | +483.33% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | -14.29% |
| RenderSink/plain_text | 0 |  | 0 | -100.00% | 0 |  | 5 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | 0% |
| Load/strings | 190 | -1.04% | 132 | +3.94% | 0 |  | 2,908 | -0.17% |
| Render/strings | 429 | +2.14% | 459 | +0.66% | 0 |  | 1,482 | +1.37% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 163 | -2.40% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 13 | +225.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 12 | +300.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 4 | +33.33% |

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
