# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (130 so far, latest [e0de206](https://github.com/jinja2cpp/Jinja2Cpp/commit/e0de206b11eab545f01c84a4f8b9ea74926ff7a2) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,535 | +0.08% |  | 55 | 0% |  | 18,304 | +0.09% | -53.16% |
| [Render/chat_llama](#renderchat_llama) | 305,953 | +0.01% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 395,013 | +0.23% |  | 51 | 0% |  | 20,776 | +0.15% | -53.47% |
| [Render/chat_mistral](#renderchat_mistral) | 508,312 | -0.08% |  | 424 | 0% |  | 9,520 | -0.17% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,870 | +0.12% |  | 44 | 0% |  | 14,448 | 0% | -47.13% |
| [Render/chat_qwen](#renderchat_qwen) | 293,530 | +0.05% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,059 | +0.17% |  | 25 | 0% |  | 5,496 | +0.29% | -47.95% |
| [Render/config_file](#renderconfig_file) | 1,257,036 | +0.02% |  | 908 | 0% |  | 23,384 | +0.27% | -1.78% |
| [Load/dict_ops](#loaddict_ops) | 33,274 | +0.12% | -77.48% | 9 | 0% | -93.13% | 2,240 | +0.72% | -47.37% |
| [Render/dict_ops](#renderdict_ops) | 296,697 | -0.34% | -57.05% | 112 | 0% | -75.11% | 24,424 | +0.07% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,488 | +0.13% | -77.93% | 16 | 0% | -89.04% | 2,792 | +0.58% | -54.50% |
| [Render/expressions](#renderexpressions) | 423,489 | 0% | -53.26% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,957 | -0.07% | -73.78% | 27 | 0% | -88.16% | 3,784 | 0% | -66.85% |
| [Render/filters](#renderfilters) | 30,174 | +0.10% | -65.97% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,059 | +0.15% | -77.80% | 7 | 0% | -94.31% | 1,968 | 0% | -42.92% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,120 | +0.18% | -68.56% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,563 | +0.09% | -77.55% | 10 | 0% | -93.63% | 2,448 | +1.32% | -45.84% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,738 | +0.31% | -81.85% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,672 | +0.18% | -75.88% | 7 | 0% | -87.50% | 1,152 | 0% | -42.40% |
| [Render/for_range](#renderfor_range) | 38,760 | +0.03% | -57.97% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,807 | +0.24% |  | 43 | 0% |  | 12,656 | +0.38% | -48.72% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,211,694 | +0.14% |  | 674 | 0% |  | 43,712 | +0.07% | +8.24% |
| [Load/inheritance](#loadinheritance) | 23,162 | 0% | -79.70% | 7 | 0% | -93.46% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 136,580 | +0.11% | -81.74% | 35 | 0% | -94.37% | 6,472 | +0.25% | +7.58% |
| [Load/int_arith](#loadint_arith) | 66,194 | +0.24% |  | 20 | 0% |  | 3,904 | +0.83% | +1.24% |
| [Render/int_arith](#renderint_arith) | 821,283 | +0.23% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,011 | 0% | -82.50% | 14 | 0% | -96.82% | 43,184 | 0% | -19.11% |
| [Render/large_static](#renderlarge_static) | 20,881 | 0% | -72.21% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,691 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,505 | +0.29% | -75.65% | 27 | 0% | -85.48% | 4,424 | +0.36% | -28.83% |
| [Render/macros](#rendermacros) | 649,130 | +0.50% | -74.67% | 308 | 0% | -88.71% | 18,864 | +0.08% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,120,989 | +0.13% | -79.35% | 951 | 0% | -97.22% | 431,184 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,734 | +0.12% | -65.35% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,359 | +0.01% | -80.49% | 19 | 0% | -89.67% | 3,424 | 0% | -43.54% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,129 | 0% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,410 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,679 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,695 | +0.01% |  | 35 | 0% |  | 5,752 | 0% | -30.40% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,175 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,478 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,712 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,747 | 0% | -61.27% | 4 | 0% | -77.78% | 736 | 0% | -34.29% |
| [Render/plain_text](#renderplain_text) | 1,013 | 0% | -85.09% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 719 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,312 | +0.09% | -75.22% | 29 | 0% | -87.05% | 4,312 | +0.37% | -52.30% |
| [Render/strings](#renderstrings) | 577,495 | 0% | -72.47% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,776 | 0% | -73.96% | 6 | 0% | -83.78% | 896 | 0% | -41.05% |
| [Render/substitute](#rendersubstitute) | 2,230 | 0% | -74.70% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,941 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 712 | -1.39% | 713 | +0.56% | 0 |  | 7,590 | +1.40% |
| Render/chat_llama | 1,146 | +4.09% | 490 | +1.03% | 0 |  | 3,581 | -2.00% |
| Load/chat_mistral | 1,037 | -0.19% | 1,003 | -0.50% | 0 |  | 8,629 | +3.83% |
| Render/chat_mistral | 1,846 | +1.37% | 527 | +2.93% | 0 |  | 9,479 | +4.68% |
| Load/chat_qwen | 465 | -1.27% | 603 | +0.50% | 0 |  | 4,564 | +7.74% |
| Render/chat_qwen | 1,043 | +0.19% | 390 | +4.56% | 0 |  | 3,355 | +8.12% |
| Load/config_file | 188 | +8.05% | 170 | 0% | 0 |  | 3,062 | +2.55% |
| Render/config_file | 6,203 | +5.48% | 1,882 | +3.69% | 0 |  | 40,333 | +6.10% |
| Load/dict_ops | 35 | 0% | 16 | +6.67% | 0 |  | 1,325 | +2.55% |
| Render/dict_ops | 708 | -4.32% | 422 | -1.86% | 0 |  | 855 | +3.14% |
| Load/expressions | 23 | -25.81% | 10 | -9.09% | 0 |  | 1,378 | +1.70% |
| Render/expressions | 6 | +200.00% | 2 |  | 0 |  | 282 | +3.30% |
| Load/filters | 125 | +2.46% | 75 | +5.63% | 0 |  | 2,437 | +13.35% |
| Render/filters | 19 | -20.83% | 5 | 0% | 0 |  | 738 | +3.07% |
| Load/for_filter_if | 12 | +140.00% | 3 | +50.00% | 0 |  | 1,084 | +1.03% |
| Render/for_filter_if | 1,648 | 0% | 69 | +2.99% | 0 |  | 167 | +17.61% |
| Load/for_loop_vars | 16 | -11.11% | 5 | -28.57% | 0 |  | 1,099 | -3.09% |
| Render/for_loop_vars | 6 | -14.29% | 3 | 0% | 0 |  | 159 | +51.43% |
| Load/for_range | 3 | 0% | 3 | 0% | 0 |  | 747 | +0.27% |
| Render/for_range | 1 |  | 1 | 0% | 0 |  | 114 | +50.00% |
| Load/html_autoescape | 476 | +2.15% | 519 | +0.78% | 0 |  | 4,195 | -4.05% |
| Render/html_autoescape | 4,178 | -8.86% | 1,397 | -0.85% | 0 |  | 31,861 | -5.73% |
| Load/inheritance | 3 | 0% | 2 | -33.33% | 0 |  | 662 | +7.99% |
| Render/inheritance | 593 | +2.24% | 120 | -0.83% | 0 |  | 850 | +1.55% |
| Load/int_arith | 77 | +10.00% | 46 | +6.98% | 0 |  | 2,122 | +1.68% |
| Render/int_arith | 11 | -47.62% | 5 | -37.50% | 0 |  | 716 | +4.99% |
| Load/large_static | 1,946 | +0.15% | 908 | 0% | 0 |  | 234 | -42.36% |
| Render/large_static | 707 | 0% | 568 | 0% | 0 |  | 4 | -20.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 686 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| Load/macros | 69 | +16.95% | 49 | +6.52% | 0 |  | 1,635 | -1.80% |
| Render/macros | 725 | +0.55% | 212 | +1.44% | 0 |  | 2,022 | +118.36% |
| Load/many_tags | 27,845 | +0.35% | 22,260 | +0.16% | 0 |  | 260,834 | -4.60% |
| Render/many_tags | 6,127 | -0.07% | 847 | +0.47% | 0 |  | 149 | -6.29% |
| Load/mitsuhiko_table | 20 | +53.85% | 22 | +46.67% | 0 |  | 836 | -1.99% |
| Render/mitsuhiko_table | 9,051 | +0.25% | 6,611 | +0.12% | 0 |  | 304 | +1.33% |
| RenderSink/mitsuhiko_table | 9,008 | +0.17% | 1,489 | -0.33% | 0 |  | 255 | -1.16% |
| RenderTo/mitsuhiko_table | 9,049 | +0.19% | 6,614 | +0.02% | 0 |  | 276 | -1.08% |
| Load/mitsuhiko_table_wide | 97 | +4.30% | 94 | +6.82% | 0 |  | 968 | +0.62% |
| Render/mitsuhiko_table_wide | 9,191 | +0.10% | 23,049 | +0.03% | 0 |  | 308 | +4.05% |
| RenderSink/mitsuhiko_table_wide | 9,099 | +0.25% | 1,538 | +0.26% | 0 |  | 249 | +4.62% |
| RenderTo/mitsuhiko_table_wide | 9,180 | +0.08% | 22,998 | 0% | 0 |  | 295 | +10.49% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 6 | -60.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 7 | 0% |
| RenderSink/plain_text | 0 |  | 1 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| Load/strings | 192 | +6.67% | 127 | +9.48% | 0 |  | 2,913 | +2.14% |
| Render/strings | 420 | +2.44% | 456 | -0.22% | 0 |  | 1,462 | +1.67% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 167 | -4.02% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |

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
