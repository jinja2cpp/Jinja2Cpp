# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (123 so far, latest [a4a1ca3](https://github.com/jinja2cpp/Jinja2Cpp/commit/a4a1ca3505c326ddaa70d5e57824903617e94551) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,865 | +0.35% |  | 55 | 0% |  | 18,272 | 0% | -53.24% |
| [Render/chat_llama](#renderchat_llama) | 306,097 | +0.01% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 395,864 | +0.32% |  | 50 | 0% |  | 20,856 | 0% | -53.29% |
| [Render/chat_mistral](#renderchat_mistral) | 512,675 | +0.04% |  | 428 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,884 | +0.26% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 293,494 | +0.03% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,950 | +0.31% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,257,333 | +0.12% |  | 908 | 0% |  | 23,320 | +0.07% | -2.05% |
| [Load/dict_ops](#loaddict_ops) | 33,270 | +0.46% | -77.49% | 9 | 0% | -93.13% | 2,208 | 0% | -48.12% |
| [Render/dict_ops](#renderdict_ops) | 296,980 | +0.06% | -57.01% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,595 | +0.38% | -77.88% | 16 | 0% | -89.04% | 2,760 | 0% | -55.02% |
| [Render/expressions](#renderexpressions) | 424,624 | +0.05% | -53.14% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,030 | +0.42% | -73.75% | 27 | 0% | -88.16% | 3,768 | 0% | -66.99% |
| [Render/filters](#renderfilters) | 30,972 | +0.01% | -65.07% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,043 | +0.38% | -77.81% | 7 | 0% | -94.31% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,788 | -0.10% | -68.60% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,561 | +0.39% | -77.55% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,207 | +0.05% | -81.90% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,652 | +0.51% | -75.92% | 7 | 0% | -87.50% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 38,753 | +0.32% | -57.97% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,693 | +0.23% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,232,071 | +0.01% |  | 734 | 0% |  | 43,680 | +0.04% | +8.16% |
| [Load/inheritance](#loadinheritance) | 23,163 | +0.29% | -79.70% | 7 | 0% | -93.46% | 1,472 | 0% | -48.75% |
| [Render/inheritance](#renderinheritance) | 136,431 | +0.18% | -81.76% | 35 | 0% | -94.37% | 6,456 | 0% | +7.31% |
| [Load/int_arith](#loadint_arith) | 66,183 | +0.28% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 821,188 | -0.02% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,110 | +0.20% | -82.49% | 14 | 0% | -96.82% | 43,168 | 0% | -19.14% |
| [Render/large_static](#renderlarge_static) | 20,893 | 0% | -72.19% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,688 | +0.01% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,349 | +0.27% | -75.72% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 648,031 | +0.02% | -74.72% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,095,024 | +0.29% | -79.41% | 951 | 0% | -97.22% | 428,800 | 0% | -59.36% |
| [Render/many_tags](#rendermany_tags) | 724,133 | 0% | -65.24% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,344 | +0.41% | -80.49% | 19 | 0% | -89.67% | 3,408 | 0% | -43.80% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,186,696 | +0.27% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,407 | +0.27% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,117 | +0.27% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,675 | +0.34% |  | 35 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,532,732 | +0.25% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,355,926 | +0.26% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,163 | +0.25% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,745 | +0.29% | -61.30% | 4 | 0% | -77.78% | 720 | 0% | -35.71% |
| [Render/plain_text](#renderplain_text) | 1,025 | +0.10% | -84.91% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 716 | +0.14% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | +0.12% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,450 | +0.43% | -75.18% | 29 | 0% | -87.05% | 4,296 | 0% | -52.48% |
| [Render/strings](#renderstrings) | 615,953 | +0.01% | -70.63% | 449 | 0% | -78.76% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,776 | +0.30% | -73.96% | 6 | 0% | -83.78% | 880 | 0% | -42.11% |
| [Render/substitute](#rendersubstitute) | 2,242 | +0.04% | -74.56% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,938 | +0.05% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | +0.05% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 690 | -1.00% | 724 | +0.70% | 0 |  | 7,235 | -0.50% |
| Render/chat_llama | 1,103 | -0.90% | 488 | +2.31% | 0 |  | 3,371 | -10.23% |
| Load/chat_mistral | 1,059 | -0.56% | 1,029 | +0.39% | 0 |  | 8,009 | -3.52% |
| Render/chat_mistral | 1,798 | +1.12% | 484 | +2.54% | 0 |  | 9,345 | +9.07% |
| Load/chat_qwen | 522 | +2.76% | 602 | +0.67% | 0 |  | 4,159 | -2.65% |
| Render/chat_qwen | 1,054 | +2.93% | 405 | +2.02% | 0 |  | 3,093 | -2.49% |
| Load/config_file | 188 | +3.30% | 164 | 0% | 0 |  | 2,863 | -0.10% |
| Render/config_file | 6,047 | +7.79% | 1,917 | +6.03% | 0 |  | 37,418 | -3.96% |
| Load/dict_ops | 40 | +17.65% | 20 | +5.26% | 0 |  | 1,280 | -1.31% |
| Render/dict_ops | 737 | +0.82% | 430 | 0% | 0 |  | 861 | +3.86% |
| Load/expressions | 38 | -5.00% | 20 | -9.09% | 0 |  | 1,300 | -3.42% |
| Render/expressions | 4 | +100.00% | 4 | +300.00% | 0 |  | 307 | +12.04% |
| Load/filters | 138 | +11.29% | 78 | +8.33% | 0 |  | 2,056 | -6.72% |
| Render/filters | 26 | +18.18% | 8 | 0% | 0 |  | 775 | +4.17% |
| Load/for_filter_if | 12 | +20.00% | 5 | 0% | 0 |  | 1,031 | -0.10% |
| Render/for_filter_if | 1,654 | +0.12% | 59 | -3.28% | 0 |  | 138 | -7.38% |
| Load/for_loop_vars | 30 | -9.09% | 12 | +9.09% | 0 |  | 1,095 | +3.69% |
| Render/for_loop_vars | 4 | 0% | 1 | 0% | 0 |  | 103 | -23.13% |
| Load/for_range | 5 | 0% | 4 | 0% | 0 |  | 726 | +3.57% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 91 | +18.18% |
| Load/html_autoescape | 482 | +1.05% | 504 | +0.40% | 0 |  | 3,827 | -6.82% |
| Render/html_autoescape | 5,601 | +25.58% | 1,576 | +11.77% | 0 |  | 29,489 | -6.79% |
| Load/inheritance | 6 | 0% | 5 | +25.00% | 0 |  | 621 | +1.64% |
| Render/inheritance | 589 | +0.68% | 121 | +3.42% | 0 |  | 838 | +2.57% |
| Load/int_arith | 90 | +9.76% | 63 | +14.55% | 0 |  | 1,962 | -6.08% |
| Render/int_arith | 9 | +125.00% | 4 | +100.00% | 0 |  | 729 | +7.05% |
| Load/large_static | 1,943 | -0.05% | 909 | -0.11% | 0 |  | 263 | -6.74% |
| Render/large_static | 708 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 2 | -33.33% |
| RenderTo/large_static | 686 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| Load/macros | 71 | +16.39% | 53 | +3.92% | 0 |  | 1,632 | +1.68% |
| Render/macros | 725 | +0.97% | 211 | -1.40% | 0 |  | 567 | +0.89% |
| Load/many_tags | 28,111 | -2.05% | 22,112 | +0.03% | 0 |  | 242,014 | -6.11% |
| Render/many_tags | 6,135 | +0.05% | 848 | -0.24% | 0 |  | 124 | -16.22% |
| Load/mitsuhiko_table | 16 | -27.27% | 16 | 0% | 0 |  | 824 | +5.91% |
| Render/mitsuhiko_table | 9,025 | 0% | 6,601 | +0.03% | 0 |  | 304 | +3.40% |
| RenderSink/mitsuhiko_table | 8,999 | +0.03% | 1,499 | +0.27% | 0 |  | 229 | -4.58% |
| RenderTo/mitsuhiko_table | 9,036 | +0.16% | 6,607 | -0.03% | 0 |  | 288 | +9.92% |
| Load/mitsuhiko_table_wide | 86 | +17.81% | 86 | +13.16% | 0 |  | 1,024 | +1.79% |
| Render/mitsuhiko_table_wide | 9,138 | -0.14% | 23,029 | +0.02% | 0 |  | 287 | -12.23% |
| RenderSink/mitsuhiko_table_wide | 9,042 | -0.06% | 1,531 | +0.59% | 0 |  | 218 | -1.36% |
| RenderTo/mitsuhiko_table_wide | 9,135 | -0.13% | 22,996 | +0.03% | 0 |  | 242 | -15.38% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 15 | -11.76% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 192 | +5.49% | 134 | +8.06% | 0 |  | 2,802 | -0.43% |
| Render/strings | 494 | +2.92% | 601 | 0% | 0 |  | 1,472 | -0.20% |
| Load/substitute | 2 | +100.00% | 3 | 0% | 0 |  | 169 | -17.56% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 1 | -66.67% |
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
