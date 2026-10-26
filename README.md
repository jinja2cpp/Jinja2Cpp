# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (146 so far, latest [2131302](https://github.com/jinja2cpp/Jinja2Cpp/commit/2131302bcbb18a57d3f7660fdae3eb4e96805fec) on 2026-10-26). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 344,744 | +0.04% |  | 55 | 0% |  | 18,256 | 0% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 306,092 | 0% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 393,627 | +0.04% |  | 52 | 0% |  | 20,728 | 0% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 508,566 | 0% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,232 | +0.03% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 293,485 | 0% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,966 | +0.05% |  | 25 | 0% |  | 5,448 | 0% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,258,866 | 0% |  | 908 | 0% |  | 23,416 | 0% | -1.65% |
| [Load/dict_ops](#loaddict_ops) | 33,191 | +0.06% | -77.54% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,103 | 0% | -57.14% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,372 | +0.03% | -77.99% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 424,637 | 0% | -53.14% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,816 | +0.01% | -73.83% | 27 | 0% | -88.16% | 3,736 | 0% | -67.27% |
| [Render/filters](#renderfilters) | 30,228 | +0.01% | -65.91% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 29,926 | +0.08% | -77.90% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,512 | 0% | -68.40% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,428 | +0.07% | -77.62% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,341 | 0% | -82.07% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,579 | +0.11% | -76.04% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,759 | +0.01% | -57.97% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,002 | +0.03% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,212,121 | 0% |  | 674 | 0% |  | 43,744 | 0% | +8.32% |
| [Load/inheritance](#loadinheritance) | 22,772 | +0.15% | -80.05% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,595 | 0% | -81.73% | 35 | 0% | -94.37% | 6,488 | 0% | +7.85% |
| [Load/int_arith](#loadint_arith) | 66,098 | +0.03% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 825,969 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,237 | +0.01% | -82.48% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,884 | +0.01% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,689 | +0.04% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | +0.01% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,506 | +0.05% | -75.65% | 27 | 0% | -85.48% | 4,376 | 0% | -29.60% |
| [Render/macros](#rendermacros) | 649,936 | 0% | -74.64% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,090,816 | +0.04% | -79.42% | 951 | 0% | -97.22% | 431,152 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,445 | 0% | -65.37% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,356 | +0.06% | -80.49% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,202 | 0% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,478 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,752 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,997 | +0.02% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,191 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,502 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,760 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,692 | +0.24% | -61.84% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,017 | +0.20% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 720 | +0.70% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | +0.25% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,305 | +0.04% | -75.23% | 30 | 0% | -86.61% | 4,264 | 0% | -52.83% |
| [Render/strings](#renderstrings) | 577,974 | 0% | -72.44% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,715 | +0.12% | -74.16% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,233 | +0.09% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,939 | +0.26% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | +0.10% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 700 | +1.60% | 727 | +0.55% | 0 |  | 7,405 | -5.15% |
| Render/chat_llama | 1,128 | -0.09% | 480 | +0.42% | 0 |  | 3,512 | +1.65% |
| Load/chat_mistral | 1,069 | +2.10% | 1,009 | -0.20% | 0 |  | 8,357 | -4.74% |
| Render/chat_mistral | 1,779 | +0.91% | 550 | +1.10% | 0 |  | 9,897 | +10.84% |
| Load/chat_qwen | 456 | +1.56% | 590 | -0.17% | 0 |  | 4,322 | -4.53% |
| Render/chat_qwen | 1,046 | +0.38% | 390 | +0.26% | 0 |  | 3,040 | -1.81% |
| Load/config_file | 178 | +1.14% | 166 | 0% | 0 |  | 2,927 | -5.55% |
| Render/config_file | 5,729 | +3.79% | 1,382 | +0.66% | 0 |  | 39,243 | -0.81% |
| Load/dict_ops | 40 | -6.98% | 22 | -12.00% | 0 |  | 1,303 | -6.66% |
| Render/dict_ops | 747 | +1.91% | 418 | -0.24% | 0 |  | 836 | +3.98% |
| Load/expressions | 17 | -5.56% | 5 | -44.44% | 0 |  | 1,394 | +4.11% |
| Render/expressions | 2 | -33.33% | 1 | 0% | 0 |  | 292 | -5.19% |
| Load/filters | 131 | -2.24% | 69 | -1.43% | 0 |  | 2,150 | +8.26% |
| Render/filters | 13 | 0% | 2 | 0% | 0 |  | 731 | +2.38% |
| Load/for_filter_if | 16 | +23.08% | 3 | 0% | 0 |  | 1,031 | -3.10% |
| Render/for_filter_if | 1,656 | +0.24% | 57 | +3.64% | 0 |  | 183 | +12.27% |
| Load/for_loop_vars | 26 | +13.04% | 12 | +9.09% | 0 |  | 1,086 | -1.09% |
| Render/for_loop_vars | 4 | 0% | 1 | 0% | 0 |  | 144 | +3.60% |
| Load/for_range | 3 | 0% | 3 | 0% | 0 |  | 715 | -2.85% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 83 | -31.40% |
| Load/html_autoescape | 453 | -0.88% | 523 | -0.19% | 0 |  | 3,893 | -5.53% |
| Render/html_autoescape | 3,265 | -1.63% | 1,063 | +1.43% | 0 |  | 31,706 | -2.11% |
| Load/inheritance | 3 | 0% | 2 | 0% | 0 |  | 647 | -3.72% |
| Render/inheritance | 583 | +0.87% | 91 | +1.11% | 0 |  | 802 | -0.99% |
| Load/int_arith | 79 | -1.25% | 53 | -10.17% | 0 |  | 2,082 | -1.42% |
| Render/int_arith | 19 | +5.56% | 6 | +20.00% | 0 |  | 716 | +0.14% |
| Load/large_static | 1,940 | +0.15% | 909 | 0% | 0 |  | 285 | -19.72% |
| Render/large_static | 706 | 0% | 569 | +0.18% | 0 |  | 5 | -58.33% |
| RenderSink/large_static | 1 | 0% | 0 | -100.00% | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 686 | -0.15% | 569 | +0.18% | 0 |  | 5 | +25.00% |
| Load/macros | 55 | -16.67% | 47 | -11.32% | 0 |  | 1,631 | -3.38% |
| Render/macros | 731 | +0.14% | 217 | +0.46% | 0 |  | 1,885 | +99.47% |
| Load/many_tags | 27,842 | +0.54% | 22,274 | +0.07% | 0 |  | 256,621 | +0.99% |
| Render/many_tags | 6,130 | +0.05% | 864 | +0.12% | 0 |  | 1,603 | +1105.26% |
| Load/mitsuhiko_table | 25 | +4.17% | 26 | -3.70% | 0 |  | 827 | -10.69% |
| Render/mitsuhiko_table | 9,053 | +0.10% | 6,610 | +0.02% | 0 |  | 318 | +6.71% |
| RenderSink/mitsuhiko_table | 9,024 | +0.17% | 1,504 | +0.80% | 0 |  | 267 | +10.79% |
| RenderTo/mitsuhiko_table | 9,048 | +0.11% | 6,605 | +0.05% | 0 |  | 288 | +2.13% |
| Load/mitsuhiko_table_wide | 74 | +5.71% | 73 | +7.35% | 0 |  | 988 | -1.00% |
| Render/mitsuhiko_table_wide | 9,251 | +0.17% | 23,070 | +0.02% | 0 |  | 304 | +8.57% |
| RenderSink/mitsuhiko_table_wide | 9,109 | +0.04% | 1,578 | -0.32% | 0 |  | 239 | -2.85% |
| RenderTo/mitsuhiko_table_wide | 9,231 | +0.23% | 23,003 | +0.02% | 0 |  | 284 | +7.17% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 24 | -4.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 8 | +14.29% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 0 |  | 1 |  | 0 |  | 7 | +16.67% |
| Load/strings | 176 | +2.92% | 110 | +1.85% | 0 |  | 2,941 | 0% |
| Render/strings | 432 | +1.17% | 453 | +0.22% | 0 |  | 1,464 | -0.14% |
| Load/substitute | 2 | 0% | 3 | +50.00% | 0 |  | 170 | -24.11% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 3 | 0% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | +50.00% |
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
