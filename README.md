# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (150 so far, latest [47d0037](https://github.com/jinja2cpp/Jinja2Cpp/commit/47d003762f009b2e2a6831caf74e990d4d4c7948) on 2026-10-28). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,103 | 0% |  | 55 | 0% |  | 18,256 | 0% | -53.29% |
| [Render/chat_llama](#renderchat_llama) | 307,511 | 0% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 394,124 | 0% |  | 52 | 0% |  | 20,728 | 0% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 510,234 | -0.01% |  | 424 | 0% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 267,169 | 0% |  | 44 | 0% |  | 14,400 | 0% | -47.31% |
| [Render/chat_qwen](#renderchat_qwen) | 294,955 | 0% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 99,294 | 0% |  | 25 | 0% |  | 5,448 | 0% | -48.41% |
| [Render/config_file](#renderconfig_file) | 1,258,106 | 0% |  | 908 | 0% |  | 23,416 | 0% | -1.65% |
| [Load/dict_ops](#loaddict_ops) | 33,197 | 0% | -77.54% | 9 | 0% | -93.13% | 2,192 | 0% | -48.50% |
| [Render/dict_ops](#renderdict_ops) | 296,084 | 0% | -57.14% | 112 | 0% | -75.11% | 24,424 | 0% | -29.61% |
| [Load/expressions](#loadexpressions) | 45,372 | 0% | -77.99% | 16 | 0% | -89.04% | 2,744 | 0% | -55.28% |
| [Render/expressions](#renderexpressions) | 424,637 | 0% | -53.14% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 71,816 | 0% | -73.83% | 27 | 0% | -88.16% | 3,736 | 0% | -67.27% |
| [Render/filters](#renderfilters) | 30,278 | -0.01% | -65.85% | 26 | 0% | -64.86% | 1,080 | 0% | -78.01% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,083 | 0% | -77.78% | 7 | 0% | -94.31% | 1,920 | 0% | -44.32% |
| [Render/for_filter_if](#renderfor_filter_if) | 273,512 | 0% | -68.40% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,524 | +0.02% | -77.57% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 192,341 | 0% | -82.07% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,579 | 0% | -76.04% | 7 | 0% | -87.50% | 1,104 | 0% | -44.80% |
| [Render/for_range](#renderfor_range) | 38,759 | 0% | -57.97% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 195,578 | 0% |  | 43 | 0% |  | 12,608 | 0% | -48.91% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,216,422 | 0% |  | 674 | 0% |  | 43,744 | 0% | +8.32% |
| [Load/inheritance](#loadinheritance) | 22,772 | 0% | -80.05% | 7 | 0% | -93.46% | 1,344 | 0% | -53.20% |
| [Render/inheritance](#renderinheritance) | 136,635 | 0% | -81.73% | 35 | 0% | -94.37% | 6,488 | 0% | +7.85% |
| [Load/int_arith](#loadint_arith) | 66,092 | 0% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 825,969 | 0% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,237 | 0% | -82.48% | 14 | 0% | -96.82% | 43,136 | 0% | -19.20% |
| [Render/large_static](#renderlarge_static) | 20,884 | 0% | -72.20% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,689 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,263 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,670 | 0% | -75.58% | 27 | 0% | -85.48% | 4,376 | 0% | -29.60% |
| [Render/macros](#rendermacros) | 649,936 | 0% | -74.64% | 308 | 0% | -88.71% | 18,864 | 0% | +28.15% |
| [Load/many_tags](#loadmany_tags) | 9,090,816 | 0% | -79.42% | 951 | 0% | -97.22% | 431,152 | 0% | -59.14% |
| [Render/many_tags](#rendermany_tags) | 721,445 | 0% | -65.37% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,356 | 0% | -80.49% | 19 | 0% | -89.67% | 3,376 | 0% | -44.33% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,187,191 | 0% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,467 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,741 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 54,997 | 0% |  | 35 | 0% |  | 5,704 | 0% | -30.98% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,533,180 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,356,491 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,749 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,692 | 0% | -61.84% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 1,017 | 0% | -85.03% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 720 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 818 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,305 | 0% | -75.23% | 30 | 0% | -86.61% | 4,264 | 0% | -52.83% |
| [Render/strings](#renderstrings) | 577,973 | 0% | -72.44% | 344 | 0% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,715 | 0% | -74.16% | 6 | 0% | -83.78% | 848 | 0% | -44.21% |
| [Render/substitute](#rendersubstitute) | 2,233 | 0% | -74.66% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,939 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,038 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 737 | +0.14% | 734 | -1.21% | 0 |  | 7,240 | -3.45% |
| Render/chat_llama | 1,139 | -1.04% | 489 | -1.41% | 0 |  | 3,632 | +7.58% |
| Load/chat_mistral | 1,094 | +1.48% | 1,017 | -0.29% | 0 |  | 8,099 | -3.65% |
| Render/chat_mistral | 1,829 | -1.88% | 553 | -1.95% | 0 |  | 9,263 | +1.41% |
| Load/chat_qwen | 469 | 0% | 594 | -0.83% | 0 |  | 4,182 | -4.72% |
| Render/chat_qwen | 1,082 | +1.88% | 404 | +1.25% | 0 |  | 3,218 | +7.70% |
| Load/config_file | 173 | -2.81% | 172 | +2.38% | 0 |  | 3,029 | +2.30% |
| Render/config_file | 5,801 | -2.37% | 1,387 | -0.14% | 0 |  | 39,602 | -0.30% |
| Load/dict_ops | 40 | -21.57% | 23 | 0% | 0 |  | 1,310 | +1.71% |
| Render/dict_ops | 743 | +0.13% | 418 | -0.48% | 0 |  | 845 | +2.30% |
| Load/expressions | 14 | +7.69% | 7 | +16.67% | 0 |  | 1,362 | -1.09% |
| Render/expressions | 2 | -33.33% | 1 | 0% | 0 |  | 298 | -9.15% |
| Load/filters | 134 | +1.52% | 76 | +2.70% | 0 |  | 2,154 | +2.82% |
| Render/filters | 13 | -18.75% | 3 | +50.00% | 0 |  | 710 | -0.84% |
| Load/for_filter_if | 10 | -9.09% | 3 | 0% | 0 |  | 1,016 | -0.88% |
| Render/for_filter_if | 1,660 | +0.06% | 58 | +1.75% | 0 |  | 182 | -1.62% |
| Load/for_loop_vars | 16 | -15.79% | 10 | 0% | 0 |  | 1,080 | +1.50% |
| Render/for_loop_vars | 8 | +100.00% | 1 | 0% | 0 |  | 104 | -16.80% |
| Load/for_range | 3 | -25.00% | 3 | 0% | 0 |  | 736 | +3.08% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 64 | -30.43% |
| Load/html_autoescape | 460 | 0% | 524 | -0.95% | 0 |  | 3,916 | -0.15% |
| Render/html_autoescape | 3,923 | -5.06% | 1,159 | -10.91% | 0 |  | 35,014 | +5.83% |
| Load/inheritance | 6 | +50.00% | 4 | +100.00% | 0 |  | 636 | +5.65% |
| Render/inheritance | 574 | -0.69% | 95 | +2.15% | 0 |  | 862 | +6.03% |
| Load/int_arith | 69 | -19.77% | 46 | -13.21% | 0 |  | 2,135 | +3.94% |
| Render/int_arith | 11 | -8.33% | 5 | +25.00% | 0 |  | 714 | +1.71% |
| Load/large_static | 1,940 | 0% | 909 | 0% | 0 |  | 248 | -3.50% |
| Render/large_static | 706 | 0% | 569 | 0% | 0 |  | 5 | +25.00% |
| RenderSink/large_static | 1 |  | 0 |  | 0 |  | 3 | 0% |
| RenderTo/large_static | 686 | 0% | 569 | 0% | 0 |  | 5 | +25.00% |
| Load/macros | 66 | -4.35% | 46 | -2.13% | 0 |  | 1,681 | +1.82% |
| Render/macros | 735 | -0.27% | 218 | +0.46% | 0 |  | 938 | -6.94% |
| Load/many_tags | 28,583 | +0.19% | 22,512 | -0.02% | 0 |  | 251,448 | -2.22% |
| Render/many_tags | 6,138 | -0.02% | 865 | 0% | 0 |  | 139 | -7.33% |
| Load/mitsuhiko_table | 22 | -26.67% | 19 | -26.92% | 0 |  | 828 | +2.22% |
| Render/mitsuhiko_table | 9,066 | -0.59% | 6,611 | -0.42% | 0 |  | 269 | -15.67% |
| RenderSink/mitsuhiko_table | 9,022 | -0.06% | 1,499 | -0.07% | 0 |  | 201 | -21.79% |
| RenderTo/mitsuhiko_table | 9,055 | -0.02% | 6,619 | +0.11% | 0 |  | 250 | -14.97% |
| Load/mitsuhiko_table_wide | 70 | -10.26% | 58 | -6.45% | 0 |  | 1,074 | +10.49% |
| Render/mitsuhiko_table_wide | 9,263 | -1.57% | 23,073 | -0.08% | 0 |  | 267 | -8.87% |
| RenderSink/mitsuhiko_table_wide | 9,118 | -0.35% | 1,577 | -1.13% | 0 |  | 210 | -10.64% |
| RenderTo/mitsuhiko_table_wide | 9,241 | -1.50% | 23,010 | -0.05% | 0 |  | 229 | -14.55% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 14 | -30.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 6 | +20.00% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderTo/plain_text | 0 |  | 1 | 0% | 0 |  | 6 | +20.00% |
| Load/strings | 169 | -8.15% | 116 | 0% | 0 |  | 2,858 | -0.17% |
| Render/strings | 443 | +2.55% | 462 | +0.65% | 0 |  | 1,457 | -16.02% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 131 | -20.61% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/substitute | 1 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/substitute | 0 | -100.00% | 0 |  | 0 |  | 4 | 0% |

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
