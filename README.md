# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (128 so far, latest [a579ba0](https://github.com/jinja2cpp/Jinja2Cpp/commit/a579ba068b5152339e584bed3c106ce389329eb8) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,215 | +0.02% |  | 55 | 0% |  | 18,272 | 0% | -53.24% |
| [Render/chat_llama](#renderchat_llama) | 305,932 | -0.04% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 394,471 | +0.01% |  | 51 | 0% |  | 20,728 | 0% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 508,282 | 0% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,542 | +0.09% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 293,374 | -0.05% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,888 | -0.01% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,256,584 | 0% |  | 908 | 0% |  | 23,320 | 0% | -2.05% |
| [Load/dict_ops](#loaddict_ops) | 33,234 | -0.09% | -77.51% | 9 | 0% | -93.13% | 2,208 | 0% | -48.12% |
| [Render/dict_ops](#renderdict_ops) | 297,698 | -0.01% | -56.91% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,429 | 0% | -77.96% | 16 | 0% | -89.04% | 2,760 | 0% | -55.02% |
| [Render/expressions](#renderexpressions) | 423,478 | 0% | -53.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,014 | 0% | -73.76% | 27 | 0% | -88.16% | 3,768 | 0% | -66.99% |
| [Render/filters](#renderfilters) | 30,960 | +0.02% | -65.08% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,015 | +0.02% | -77.83% | 7 | 0% | -94.31% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,642 | 0% | -68.61% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,532 | 0% | -77.56% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,138 | -0.03% | -81.90% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,648 | 0% | -75.92% | 7 | 0% | -87.50% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 38,749 | 0% | -57.98% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,405 | 0% |  | 43 | 0% |  | 12,592 | 0% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,170 | 0% |  | 674 | 0% |  | 43,680 | 0% | +8.16% |
| [Load/inheritance](#loadinheritance) | 23,162 | 0% | -79.70% | 7 | 0% | -93.46% | 1,472 | 0% | -48.75% |
| [Render/inheritance](#renderinheritance) | 136,426 | +0.01% | -81.76% | 35 | 0% | -94.37% | 6,456 | 0% | +7.31% |
| [Load/int_arith](#loadint_arith) | 66,033 | -0.01% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 819,382 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,062 | 0% | -82.50% | 14 | 0% | -96.82% | 43,168 | 0% | -19.14% |
| [Render/large_static](#renderlarge_static) | 20,881 | 0% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,691 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,353 | 0% | -75.72% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 645,919 | -0.12% | -74.80% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,109,230 | 0% | -79.37% | 951 | 0% | -97.22% | 431,184 | -0.01% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 720,834 | 0% | -65.39% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,353 | 0% | -80.49% | 19 | 0% | -89.67% | 3,408 | 0% | -43.80% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,129 | +0.01% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,410 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,679 | +0.01% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,689 | 0% |  | 35 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,186 | +0.01% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,489 | +0.01% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,723 | +0.01% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,747 | 0% | -61.27% | 4 | 0% | -77.78% | 720 | 0% | -35.71% |
| [Render/plain_text](#renderplain_text) | 1,013 | 0% | -85.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 719 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,250 | 0% | -75.24% | 29 | 0% | -87.05% | 4,280 | 0% | -52.65% |
| [Render/strings](#renderstrings) | 577,480 | 0% | -72.47% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,776 | 0% | -73.96% | 6 | 0% | -83.78% | 880 | 0% | -42.11% |
| [Render/substitute](#rendersubstitute) | 2,230 | 0% | -74.70% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,941 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 722 | +5.56% | 716 | +0.14% | 0 |  | 7,283 | -3.60% |
| Render/chat_llama | 1,114 | +0.63% | 486 | -1.22% | 0 |  | 3,503 | -2.48% |
| Load/chat_mistral | 1,011 | -2.60% | 1,010 | -1.17% | 0 |  | 8,034 | -6.07% |
| Render/chat_mistral | 1,890 | +4.07% | 522 | +4.61% | 0 |  | 9,153 | -4.12% |
| Load/chat_qwen | 460 | -10.51% | 590 | -1.01% | 0 |  | 4,270 | -4.41% |
| Render/chat_qwen | 1,037 | -2.54% | 378 | -5.50% | 0 |  | 3,137 | -1.32% |
| Load/config_file | 166 | -3.49% | 166 | +2.47% | 0 |  | 2,990 | -0.23% |
| Render/config_file | 5,611 | +0.14% | 1,791 | -1.21% | 0 |  | 39,407 | -1.20% |
| Load/dict_ops | 32 | -3.03% | 17 | -10.53% | 0 |  | 1,317 | +1.62% |
| Render/dict_ops | 743 | -0.80% | 428 | -0.23% | 0 |  | 822 | -5.41% |
| Load/expressions | 28 | -17.65% | 11 | -35.29% | 0 |  | 1,355 | +0.44% |
| Render/expressions | 2 | -33.33% | 1 | -66.67% | 0 |  | 301 | -4.44% |
| Load/filters | 120 | -10.45% | 72 | -10.00% | 0 |  | 2,131 | +1.77% |
| Render/filters | 19 | -48.65% | 7 | -30.00% | 0 |  | 763 | -1.80% |
| Load/for_filter_if | 5 | -68.75% | 2 | -50.00% | 0 |  | 1,099 | +0.92% |
| Render/for_filter_if | 1,650 | 0% | 66 | +10.00% | 0 |  | 172 | +18.62% |
| Load/for_loop_vars | 11 | -42.11% | 7 | -30.00% | 0 |  | 1,149 | -0.43% |
| Render/for_loop_vars | 6 | -25.00% | 3 | +50.00% | 0 |  | 149 | +11.19% |
| Load/for_range | 6 | +20.00% | 5 | 0% | 0 |  | 763 | +6.27% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 94 | +2.17% |
| Load/html_autoescape | 464 | -2.52% | 509 | +0.59% | 0 |  | 4,387 | +0.83% |
| Render/html_autoescape | 4,665 | -0.85% | 1,413 | +5.29% | 0 |  | 32,309 | -4.92% |
| Load/inheritance | 3 | -57.14% | 2 | -60.00% | 0 |  | 654 | +0.31% |
| Render/inheritance | 585 | +0.86% | 120 | -1.64% | 0 |  | 852 | -0.58% |
| Load/int_arith | 68 | -15.00% | 47 | -7.84% | 0 |  | 2,111 | +2.78% |
| Render/int_arith | 13 | -35.00% | 5 | -37.50% | 0 |  | 672 | -2.04% |
| Load/large_static | 1,942 | 0% | 910 | 0% | 0 |  | 280 | +4.48% |
| Render/large_static | 707 | 0% | 568 | 0% | 0 |  | 12 | -42.86% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 686 | 0% | 568 | 0% | 0 |  | 4 | +33.33% |
| Load/macros | 77 | -1.28% | 50 | +6.38% | 0 |  | 1,676 | +0.30% |
| Render/macros | 721 | +0.14% | 209 | -0.95% | 0 |  | 1,340 | -45.31% |
| Load/many_tags | 27,743 | -2.32% | 22,205 | +0.09% | 0 |  | 254,591 | -3.70% |
| Render/many_tags | 6,133 | +0.02% | 851 | +0.47% | 0 |  | 184 | +8.88% |
| Load/mitsuhiko_table | 10 | -44.44% | 14 | -6.67% | 0 |  | 868 | +0.46% |
| Render/mitsuhiko_table | 9,029 | +0.09% | 6,604 | -0.03% | 0 |  | 323 | +1.25% |
| RenderSink/mitsuhiko_table | 8,994 | -0.06% | 1,492 | -0.20% | 0 |  | 259 | -1.52% |
| RenderTo/mitsuhiko_table | 9,033 | +0.12% | 6,613 | +0.08% | 0 |  | 289 | -1.37% |
| Load/mitsuhiko_table_wide | 80 | -5.88% | 76 | 0% | 0 |  | 990 | +6.34% |
| Render/mitsuhiko_table_wide | 9,182 | +0.37% | 23,040 | -0.02% | 0 |  | 314 | -6.55% |
| RenderSink/mitsuhiko_table_wide | 9,076 | +0.31% | 1,534 | +0.07% | 0 |  | 265 | -0.38% |
| RenderTo/mitsuhiko_table_wide | 9,175 | +0.31% | 22,999 | +0.01% | 0 |  | 288 | -6.80% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -72.73% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| Load/strings | 177 | -0.56% | 121 | +2.54% | 0 |  | 2,821 | -0.53% |
| Render/strings | 418 | +1.70% | 460 | +0.22% | 0 |  | 1,487 | -56.90% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 164 | -3.53% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 6 | -40.00% |
| RenderSink/substitute | 0 | -100.00% | 0 |  | 0 |  | 4 | +100.00% |
| RenderTo/substitute | 0 | -100.00% | 0 |  | 0 |  | 4 | +100.00% |

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
