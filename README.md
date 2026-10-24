# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (132 so far, latest [00b03d2](https://github.com/jinja2cpp/Jinja2Cpp/commit/00b03d2b87d9e27612a475e7e02d62143346d663) on 2026-10-25). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 346,797 | +0.03% |  | 55 | 0% |  | 18,304 | 0% | -53.16% |
| [Render/chat_llama](#renderchat_llama) | 306,065 | +0.01% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 395,536 | -0.03% |  | 52 | 0% |  | 20,776 | 0% | -53.47% |
| [Render/chat_mistral](#renderchat_mistral) | 508,737 | +0.06% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 268,706 | +0.03% |  | 44 | 0% |  | 14,448 | 0% | -47.13% |
| [Render/chat_qwen](#renderchat_qwen) | 293,568 | +0.01% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,323 | +0.11% |  | 25 | 0% |  | 5,496 | 0% | -47.95% |
| [Render/config_file](#renderconfig_file) | 1,258,535 | +0.06% |  | 908 | 0% |  | 23,400 | +0.07% | -1.71% |
| [Load/dict_ops](#loaddict_ops) | 33,333 | +0.02% | -77.44% | 9 | 0% | -93.13% | 2,240 | 0% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 296,904 | +0.08% | -57.02% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,510 | 0% | -77.92% | 16 | 0% | -89.04% | 2,792 | 0% | -54.50% |
| [Render/expressions](#renderexpressions) | 423,680 | +0.05% | -53.24% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,046 | +0.14% | -73.75% | 27 | 0% | -88.16% | 3,784 | 0% | -66.85% |
| [Render/filters](#renderfilters) | 30,235 | +0.06% | -65.90% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,140 | +0.12% | -77.74% | 7 | 0% | -94.31% | 1,968 | 0% | -42.92% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,261 | +0.05% | -68.54% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,683 | +0.16% | -77.48% | 10 | 0% | -93.63% | 2,448 | 0% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,444 | -1.18% | -82.06% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,687 | +0.06% | -75.85% | 7 | 0% | -87.50% | 1,152 | 0% | -42.40% |
| [Render/for_range](#renderfor_range) | 38,858 | +0.29% | -57.86% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,572 | +0.22% |  | 43 | 0% |  | 12,656 | 0% | -48.72% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,212,579 | +0.04% |  | 674 | 0% |  | 43,728 | +0.04% | +8.28% |
| [Load/inheritance](#loadinheritance) | 22,908 | -1.11% | -79.93% | 7 | 0% | -93.46% | 1,392 | -6.45% | -51.53% |
| [Render/inheritance](#renderinheritance) | 136,820 | +0.18% | -81.70% | 35 | 0% | -94.37% | 6,472 | 0% | +7.58% |
| [Load/int_arith](#loadint_arith) | 66,350 | +0.06% |  | 20 | 0% |  | 3,904 | 0% | +1.24% |
| [Render/int_arith](#renderint_arith) | 822,401 | +0.14% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,521 | +0.36% | -82.45% | 14 | 0% | -96.82% | 43,184 | 0% | -19.11% |
| [Render/large_static](#renderlarge_static) | 20,885 | +0.02% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,695 | +0.03% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,264 | +0.02% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,704 | +0.32% | -75.56% | 27 | 0% | -85.48% | 4,424 | 0% | -28.83% |
| [Render/macros](#rendermacros) | 649,036 | -0.01% | -74.68% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,134,581 | +0.10% | -79.32% | 951 | 0% | -97.22% | 431,184 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 722,338 | 0% | -65.32% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,473 | +0.21% | -80.44% | 19 | 0% | -89.67% | 3,424 | 0% | -43.54% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,200,143 | +0.25% | -57.91% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,127,424 | +0.25% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,199,693 | +0.25% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,743 | +0.06% |  | 35 | 0% |  | 5,752 | 0% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,546,189 | +0.24% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,369,492 | +0.24% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,545,726 | +0.24% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,757 | +0.27% | -61.17% | 4 | 0% | -77.78% | 736 | 0% | -34.29% |
| [Render/plain_text](#renderplain_text) | 1,017 | +0.39% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 723 | +0.56% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | +0.49% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,555 | +0.06% | -75.14% | 30 | 0% | -86.61% | 4,312 | 0% | -52.30% |
| [Render/strings](#renderstrings) | 578,522 | +0.10% | -72.42% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,786 | +0.15% | -73.92% | 6 | 0% | -83.78% | 896 | 0% | -41.05% |
| [Render/substitute](#rendersubstitute) | 2,234 | +0.18% | -74.65% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,945 | +0.21% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,039 | +0.20% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 735 | +0.68% | 718 | -0.83% | 0 |  | 7,810 | -0.66% |
| Render/chat_llama | 1,153 | +2.58% | 489 | -1.21% | 0 |  | 3,724 | +4.87% |
| Load/chat_mistral | 1,052 | -0.47% | 1,011 | +0.90% | 0 |  | 8,784 | -3.31% |
| Render/chat_mistral | 1,832 | -1.61% | 539 | +2.28% | 0 |  | 8,906 | -4.64% |
| Load/chat_qwen | 475 | +1.71% | 607 | 0% | 0 |  | 4,685 | -6.15% |
| Render/chat_qwen | 1,049 | -1.96% | 397 | 0% | 0 |  | 3,120 | -0.61% |
| Load/config_file | 187 | -2.60% | 161 | -4.73% | 0 |  | 3,159 | -0.57% |
| Render/config_file | 5,763 | -0.91% | 1,407 | -27.40% | 0 |  | 40,062 | +3.45% |
| Load/dict_ops | 45 | +12.50% | 20 | -9.09% | 0 |  | 1,331 | +0.45% |
| Render/dict_ops | 718 | +3.31% | 421 | -1.41% | 0 |  | 843 | -0.71% |
| Load/expressions | 46 | +12.20% | 21 | -8.70% | 0 |  | 1,341 | -4.69% |
| Render/expressions | 3 | 0% | 1 | 0% | 0 |  | 252 | -8.70% |
| Load/filters | 133 | -5.00% | 81 | -8.99% | 0 |  | 2,135 | +0.80% |
| Render/filters | 30 | +66.67% | 8 | +60.00% | 0 |  | 735 | +2.08% |
| Load/for_filter_if | 20 | +66.67% | 7 | +40.00% | 0 |  | 1,136 | -2.15% |
| Render/for_filter_if | 1,655 | +0.36% | 64 | +1.59% | 0 |  | 191 | -10.33% |
| Load/for_loop_vars | 30 | +7.14% | 10 | +11.11% | 0 |  | 1,199 | +0.76% |
| Render/for_loop_vars | 4 | -33.33% | 1 | -66.67% | 0 |  | 143 | -23.12% |
| Load/for_range | 3 | 0% | 3 | 0% | 0 |  | 731 | -2.79% |
| Render/for_range | 0 | -100.00% | 1 | 0% | 0 |  | 119 | -28.74% |
| Load/html_autoescape | 491 | +1.66% | 521 | +0.77% | 0 |  | 4,382 | -8.90% |
| Render/html_autoescape | 5,078 | +14.78% | 1,648 | +12.57% | 0 |  | 33,147 | -3.07% |
| Load/inheritance | 2 | -33.33% | 2 | 0% | 0 |  | 660 | +4.43% |
| Render/inheritance | 571 | -3.06% | 91 | -24.17% | 0 |  | 857 | +5.02% |
| Load/int_arith | 87 | +4.82% | 58 | +1.75% | 0 |  | 2,082 | -10.53% |
| Render/int_arith | 8 | -50.00% | 3 | -62.50% | 0 |  | 712 | +6.27% |
| Load/large_static | 1,938 | -0.05% | 909 | 0% | 0 |  | 278 | +14.40% |
| Render/large_static | 705 | -0.28% | 570 | +0.35% | 0 |  | 3 | -40.00% |
| RenderSink/large_static | 0 | -100.00% | 0 |  | 0 |  | 1 | -50.00% |
| RenderTo/large_static | 684 | -0.29% | 571 | +0.53% | 0 |  | 3 | -40.00% |
| Load/macros | 67 | -1.47% | 56 | -5.08% | 0 |  | 1,739 | +2.90% |
| Render/macros | 725 | -0.14% | 216 | +1.89% | 0 |  | 688 | +29.32% |
| Load/many_tags | 27,899 | +0.52% | 22,298 | +0.09% | 0 |  | 258,172 | -3.70% |
| Render/many_tags | 6,146 | +0.44% | 873 | +2.34% | 0 |  | 141 | -11.88% |
| Load/mitsuhiko_table | 24 | +9.09% | 21 | 0% | 0 |  | 912 | +11.08% |
| Render/mitsuhiko_table | 9,045 | +0.14% | 6,610 | -0.06% | 0 |  | 329 | -5.19% |
| RenderSink/mitsuhiko_table | 9,018 | +0.26% | 1,491 | 0% | 0 |  | 250 | -12.89% |
| RenderTo/mitsuhiko_table | 9,041 | +0.12% | 6,615 | -0.11% | 0 |  | 284 | -13.15% |
| Load/mitsuhiko_table_wide | 91 | -2.15% | 90 | -5.26% | 0 |  | 978 | -4.96% |
| Render/mitsuhiko_table_wide | 9,227 | +0.11% | 23,058 | +0.06% | 0 |  | 316 | -3.36% |
| RenderSink/mitsuhiko_table_wide | 9,104 | -0.04% | 1,572 | +2.61% | 0 |  | 233 | -13.38% |
| RenderTo/mitsuhiko_table_wide | 9,204 | -0.02% | 23,024 | +0.13% | 0 |  | 285 | -10.38% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 5 | -85.71% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -40.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 4 | -33.33% |
| Load/strings | 180 | -5.26% | 122 | -7.58% | 0 |  | 2,891 | -0.58% |
| Render/strings | 416 | -3.03% | 452 | -1.53% | 0 |  | 1,468 | -0.94% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 156 | -4.29% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 2 | -84.62% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 1 | -91.67% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 2 | -50.00% |

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
