# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (122 so far, latest [631c9ed](https://github.com/jinja2cpp/Jinja2Cpp/commit/631c9ed9b9696813ccc7e555e18982b7a8a9be62) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,651 | +0.08% |  | 55 | 0% |  | 18,272 | 0% | -53.24% |
| [Render/chat_llama](#renderchat_llama) | 306,057 | +0.05% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 394,602 | -0.03% |  | 50 | 0% |  | 20,856 | 0% | -53.29% |
| [Render/chat_mistral](#renderchat_mistral) | 512,478 | -0.01% |  | 428 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,186 | +0.02% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 293,408 | +0.02% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,645 | +0.27% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,255,799 | +0.06% |  | 908 | 0% |  | 23,304 | 0% | -2.12% |
| [Load/dict_ops](#loaddict_ops) | 33,118 | -0.02% | -77.59% | 9 | 0% | -93.13% | 2,208 | 0% | -48.12% |
| [Render/dict_ops](#renderdict_ops) | 296,801 | -0.04% | -57.04% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,421 | -0.05% | -77.97% | 16 | 0% | -89.04% | 2,760 | 0% | -55.02% |
| [Render/expressions](#renderexpressions) | 424,407 | -0.41% | -53.16% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,731 | -0.12% | -73.86% | 27 | 0% | -88.16% | 3,768 | 0% | -66.99% |
| [Render/filters](#renderfilters) | 30,969 | -0.01% | -65.07% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,930 | -0.10% | -77.89% | 7 | 0% | -94.31% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 272,061 | +0.16% | -68.57% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,405 | -0.10% | -77.63% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,103 | -0.30% | -81.91% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,583 | -0.35% | -76.04% | 7 | 0% | -87.50% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 38,629 | -0.56% | -58.11% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,249 | -0.01% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,231,915 | +0.26% |  | 734 | 0% |  | 43,664 | 0% | +8.12% |
| [Load/inheritance](#loadinheritance) | 23,096 | -0.12% | -79.76% | 7 | 0% | -93.46% | 1,472 | 0% | -48.75% |
| [Render/inheritance](#renderinheritance) | 136,189 | -0.28% | -81.79% | 35 | 0% | -94.37% | 6,456 | 0% | +7.31% |
| [Load/int_arith](#loadint_arith) | 66,000 | -0.09% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 821,351 | -0.16% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 154,799 | +0.07% | -82.53% | 14 | 0% | -96.82% | 43,168 | 0% | -19.14% |
| [Render/large_static](#renderlarge_static) | 20,892 | -0.79% | -72.19% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,687 |  |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,259 |  |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,199 | -0.11% | -75.78% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 647,927 | +0.05% | -74.72% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,068,913 | -0.01% | -79.47% | 951 | 0% | -97.22% | 428,800 | 0% | -59.36% |
| [Render/many_tags](#rendermany_tags) | 724,132 | -0.11% | -65.24% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,156 | -0.08% | -80.57% | 19 | 0% | -89.67% | 3,408 | 0% | -43.80% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,172,724 | -0.21% | -58.14% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,100,435 |  |  | 32 |  |  | 28,824 |  | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,172,145 |  |  | 32 |  |  | 28,824 |  | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,487 | -0.27% |  | 35 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,518,760 | -0.18% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,341,954 |  |  | 32 |  |  | 28,824 |  | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,518,191 |  |  | 32 |  |  | 28,824 |  | 0% |
| [Load/plain_text](#loadplain_text) | 3,734 | -1.26% | -61.41% | 4 | 0% | -77.78% | 720 | 0% | -35.71% |
| [Render/plain_text](#renderplain_text) | 1,024 | -0.78% | -84.93% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 715 |  |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 813 |  |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,140 | -0.06% | -75.28% | 29 | 0% | -87.05% | 4,296 | 0% | -52.48% |
| [Render/strings](#renderstrings) | 615,914 | -0.02% | -70.64% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,753 | -0.61% | -74.03% | 6 | 0% | -83.78% | 880 | 0% | -42.11% |
| [Render/substitute](#rendersubstitute) | 2,241 | -1.28% | -74.57% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,937 |  |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,034 |  |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 697 | -0.71% | 719 | +0.56% | 0 |  | 7,271 | +0.85% |
| Render/chat_llama | 1,113 | +0.27% | 477 | +0.85% | 0 |  | 3,755 | +4.45% |
| Load/chat_mistral | 1,065 | -2.02% | 1,025 | 0% | 0 |  | 8,301 | +1.67% |
| Render/chat_mistral | 1,778 | -2.52% | 472 | -12.27% | 0 |  | 8,568 | -2.85% |
| Load/chat_qwen | 508 | +2.63% | 598 | +2.22% | 0 |  | 4,272 | -2.86% |
| Render/chat_qwen | 1,024 | +2.61% | 397 | +2.85% | 0 |  | 3,172 | +2.95% |
| Load/config_file | 182 | +0.55% | 164 | +4.46% | 0 |  | 2,866 | -0.76% |
| Render/config_file | 5,610 | -12.10% | 1,808 | -5.64% | 0 |  | 38,959 | +3.15% |
| Load/dict_ops | 34 | +6.25% | 19 | -13.64% | 0 |  | 1,297 | +3.02% |
| Render/dict_ops | 731 | -2.40% | 430 | -2.27% | 0 |  | 829 | -1.31% |
| Load/expressions | 40 | +21.21% | 22 | +100.00% | 0 |  | 1,346 | +1.51% |
| Render/expressions | 2 | -66.67% | 1 | -66.67% | 0 |  | 274 | +2.62% |
| Load/filters | 124 | -8.15% | 72 | +4.35% | 0 |  | 2,204 | -1.65% |
| Render/filters | 22 | -24.14% | 8 | -11.11% | 0 |  | 744 | -2.23% |
| Load/for_filter_if | 10 | +66.67% | 5 | +150.00% | 0 |  | 1,032 | -0.67% |
| Render/for_filter_if | 1,652 | +0.24% | 61 | -3.17% | 0 |  | 149 | +0.68% |
| Load/for_loop_vars | 33 | +120.00% | 11 | +120.00% | 0 |  | 1,056 | +0.48% |
| Render/for_loop_vars | 4 | 0% | 1 | -50.00% | 0 |  | 134 | +2.29% |
| Load/for_range | 5 | +66.67% | 4 | +33.33% | 0 |  | 701 | +2.79% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 77 | -13.48% |
| Load/html_autoescape | 477 | +1.92% | 502 | -1.18% | 0 |  | 4,107 | +5.28% |
| Render/html_autoescape | 4,460 | +3.58% | 1,410 | +3.68% | 0 |  | 31,637 | +1.95% |
| Load/inheritance | 6 | +200.00% | 4 | +100.00% | 0 |  | 611 | +0.49% |
| Render/inheritance | 585 | +0.17% | 117 | -9.30% | 0 |  | 817 | -0.61% |
| Load/int_arith | 82 | +15.49% | 55 | +22.22% | 0 |  | 2,089 | +6.26% |
| Render/int_arith | 4 | -73.33% | 2 | -60.00% | 0 |  | 681 | +2.41% |
| Load/large_static | 1,944 | +0.10% | 910 | +0.33% | 0 |  | 282 | 0% |
| Render/large_static | 708 | +0.71% | 568 | -0.70% | 0 |  | 4 | -55.56% |
| RenderSink/large_static | 1 |  | 0 |  | 0 |  | 3 |  |
| RenderTo/large_static | 686 |  | 568 |  | 0 |  | 4 |  |
| Load/macros | 61 | +12.96% | 51 | +24.39% | 0 |  | 1,605 | +0.94% |
| Render/macros | 718 | -2.58% | 214 | -15.42% | 0 |  | 562 | -70.87% |
| Load/many_tags | 28,699 | +2.00% | 22,106 | 0% | 0 |  | 257,766 | +3.83% |
| Render/many_tags | 6,132 | -0.55% | 850 | -6.18% | 0 |  | 148 | +18.40% |
| Load/mitsuhiko_table | 22 | +144.44% | 16 | +77.78% | 0 |  | 778 | -2.26% |
| Render/mitsuhiko_table | 9,025 | -0.08% | 6,599 | -0.93% | 0 |  | 294 | -11.45% |
| RenderSink/mitsuhiko_table | 8,996 |  | 1,495 |  | 0 |  | 240 |  |
| RenderTo/mitsuhiko_table | 9,022 |  | 6,609 |  | 0 |  | 262 |  |
| Load/mitsuhiko_table_wide | 73 | -5.19% | 76 | 0% | 0 |  | 1,006 | +4.47% |
| Render/mitsuhiko_table_wide | 9,151 | +0.13% | 23,025 | +0.19% | 0 |  | 327 | +3.81% |
| RenderSink/mitsuhiko_table_wide | 9,047 |  | 1,522 |  | 0 |  | 221 |  |
| RenderTo/mitsuhiko_table_wide | 9,147 |  | 22,989 |  | 0 |  | 286 |  |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 17 | +21.43% |
| Render/plain_text | 0 |  | 0 | -100.00% | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 |  |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 |  |
| Load/strings | 182 | -1.09% | 124 | +9.73% | 0 |  | 2,814 | -0.04% |
| Render/strings | 480 | -0.21% | 601 | -0.50% | 0 |  | 1,475 | +1.30% |
| Load/substitute | 1 | -50.00% | 3 | 0% | 0 |  | 205 | +15.82% |
| Render/substitute | 0 |  | 0 | -100.00% | 0 |  | 4 | +33.33% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 |  |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 3 |  |

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
