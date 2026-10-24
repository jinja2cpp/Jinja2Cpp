# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (124 so far, latest [8cb1c71](https://github.com/jinja2cpp/Jinja2Cpp/commit/8cb1c7190cb51ac9e3e6e4c003442dfb02548351) on 2026-10-24). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 345,195 | -0.19% |  | 55 | 0% |  | 18,272 | 0% | -53.24% |
| [Render/chat_llama](#renderchat_llama) | 306,070 | -0.01% |  | 251 | 0% |  | 11,136 | 0% | -22.45% |
| [Load/chat_mistral](#loadchat_mistral) | 394,476 | -0.35% |  | 51 | +2.00% |  | 20,728 | -0.61% | -53.57% |
| [Render/chat_mistral](#renderchat_mistral) | 508,289 | -0.86% |  | 424 | -0.93% |  | 9,520 | 0% | -27.92% |
| [Load/chat_qwen](#loadchat_qwen) | 266,362 | -0.20% |  | 44 | 0% |  | 14,432 | 0% | -47.19% |
| [Render/chat_qwen](#renderchat_qwen) | 293,523 | +0.01% |  | 238 | 0% |  | 7,824 | 0% | -8.00% |
| [Load/config_file](#loadconfig_file) | 98,902 | -0.05% |  | 25 | 0% |  | 5,464 | 0% | -48.26% |
| [Render/config_file](#renderconfig_file) | 1,257,261 | -0.01% |  | 908 | 0% |  | 23,320 | 0% | -2.05% |
| [Load/dict_ops](#loaddict_ops) | 33,269 | 0% | -77.49% | 9 | 0% | -93.13% | 2,208 | 0% | -48.12% |
| [Render/dict_ops](#renderdict_ops) | 296,980 | 0% | -57.01% | 112 | 0% | -75.11% | 24,408 | 0% | -29.65% |
| [Load/expressions](#loadexpressions) | 45,427 | -0.37% | -77.96% | 16 | 0% | -89.04% | 2,760 | 0% | -55.02% |
| [Render/expressions](#renderexpressions) | 423,475 | -0.27% | -53.27% | 3 | 0% | -90.00% | 2,440 | 0% | -31.15% |
| [Load/filters](#loadfilters) | 72,005 | -0.03% | -73.76% | 27 | 0% | -88.16% | 3,768 | 0% | -66.99% |
| [Render/filters](#renderfilters) | 30,972 | 0% | -65.07% | 29 | 0% | -60.81% | 1,376 | 0% | -71.99% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,011 | -0.11% | -77.83% | 7 | 0% | -94.31% | 1,952 | 0% | -43.39% |
| [Render/for_filter_if](#renderfor_filter_if) | 271,649 | -0.05% | -68.61% | 4 | 0% | -98.23% | 13,328 | 0% | +266.15% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 40,533 | -0.07% | -77.56% | 10 | 0% | -93.63% | 2,400 | 0% | -46.90% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 194,207 | 0% | -81.90% | 2 | 0% | -99.62% | 1,552 | 0% | -43.93% |
| [Load/for_range](#loadfor_range) | 13,646 | -0.04% | -75.93% | 7 | 0% | -87.50% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 38,753 | 0% | -57.97% | 3 | 0% | -88.89% | 408 | 0% | -73.02% |
| [Load/html_autoescape](#loadhtml_autoescape) | 194,383 | -0.16% |  | 43 | 0% |  | 12,592 | -0.13% | -48.98% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,210,164 | -1.78% |  | 674 | -8.17% |  | 43,680 | 0% | +8.16% |
| [Load/inheritance](#loadinheritance) | 23,160 | -0.01% | -79.71% | 7 | 0% | -93.46% | 1,472 | 0% | -48.75% |
| [Render/inheritance](#renderinheritance) | 136,431 | 0% | -81.76% | 35 | 0% | -94.37% | 6,456 | 0% | +7.31% |
| [Load/int_arith](#loadint_arith) | 66,031 | -0.23% |  | 20 | 0% |  | 3,856 | 0% | 0% |
| [Render/int_arith](#renderint_arith) | 819,386 | -0.22% |  | 12 | 0% |  | 3,288 | 0% | 0% |
| [Load/large_static](#loadlarge_static) | 155,060 | -0.03% | -82.50% | 14 | 0% | -96.82% | 43,168 | 0% | -19.14% |
| [Render/large_static](#renderlarge_static) | 20,893 | 0% | -72.19% | 1 | 0% | -94.44% | 36,552 | 0% | -1.83% |
| [RenderSink/large_static](#rendersinklarge_static) | 12,688 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/large_static](#rendertolarge_static) | 20,260 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/macros](#loadmacros) | 55,386 | +0.07% | -75.70% | 27 | 0% | -85.48% | 4,392 | 0% | -29.34% |
| [Render/macros](#rendermacros) | 646,931 | -0.17% | -74.76% | 308 | 0% | -88.71% | 18,848 | 0% | +28.04% |
| [Load/many_tags](#loadmany_tags) | 9,111,086 | +0.18% | -79.37% | 951 | 0% | -97.22% | 431,232 | +0.57% | -59.13% |
| [Render/many_tags](#rendermany_tags) | 720,823 | -0.46% | -65.39% | 2 | 0% | -90.48% | 5,632 | 0% | -10.77% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 46,351 | +0.02% | -80.49% | 19 | 0% | -89.67% | 3,408 | 0% | -43.80% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 5,186,696 | 0% | -58.02% | 33 | 0% | -99.19% | 373,296 | 0% | +7.58% |
| [RenderSink/mitsuhiko_table](#rendersinkmitsuhiko_table) | 5,114,407 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table](#rendertomitsuhiko_table) | 5,186,117 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 55,682 | +0.01% |  | 35 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 5,532,732 | 0% |  | 33 | 0% |  | 1,406,688 | 0% | +1.86% |
| [RenderSink/mitsuhiko_table_wide](#rendersinkmitsuhiko_table_wide) | 5,355,926 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [RenderTo/mitsuhiko_table_wide](#rendertomitsuhiko_table_wide) | 5,532,163 | 0% |  | 32 | 0% |  | 28,824 | 0% | 0% |
| [Load/plain_text](#loadplain_text) | 3,745 | 0% | -61.30% | 4 | 0% | -77.78% | 720 | 0% | -35.71% |
| [Render/plain_text](#renderplain_text) | 1,025 | 0% | -84.91% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/plain_text](#rendersinkplain_text) | 716 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/plain_text](#rendertoplain_text) | 814 | 0% |  | 0 |  |  | 0 |  |  |
| [Load/strings](#loadstrings) | 72,253 | -0.27% | -75.24% | 29 | 0% | -87.05% | 4,280 | -0.37% | -52.65% |
| [Render/strings](#renderstrings) | 577,492 | -6.24% | -72.47% | 344 | -23.39% | -83.73% | 9,104 | 0% | -49.22% |
| [Load/substitute](#loadsubstitute) | 7,774 | -0.03% | -73.96% | 6 | 0% | -83.78% | 880 | 0% | -42.11% |
| [Render/substitute](#rendersubstitute) | 2,242 | 0% | -74.56% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [RenderSink/substitute](#rendersinksubstitute) | 1,938 | 0% |  | 0 |  |  | 0 |  |  |
| [RenderTo/substitute](#rendertosubstitute) | 2,035 | 0% |  | 0 |  |  | 0 |  |  |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 696 | +0.87% | 716 | -1.10% | 0 |  | 7,573 | +4.67% |
| Render/chat_llama | 1,097 | -0.54% | 478 | -2.05% | 0 |  | 3,604 | +6.91% |
| Load/chat_mistral | 1,067 | +0.76% | 1,021 | -0.78% | 0 |  | 8,621 | +7.64% |
| Render/chat_mistral | 1,835 | +2.06% | 488 | +0.83% | 0 |  | 9,203 | -1.52% |
| Load/chat_qwen | 533 | +2.11% | 588 | -2.33% | 0 |  | 4,629 | +11.30% |
| Render/chat_qwen | 1,054 | 0% | 405 | 0% | 0 |  | 3,132 | +1.26% |
| Load/config_file | 184 | -2.13% | 165 | +0.61% | 0 |  | 2,957 | +3.28% |
| Render/config_file | 5,993 | -0.89% | 1,864 | -2.76% | 0 |  | 37,022 | -1.06% |
| Load/dict_ops | 40 | 0% | 17 | -15.00% | 0 |  | 1,301 | +1.64% |
| Render/dict_ops | 737 | 0% | 428 | -0.47% | 0 |  | 861 | 0% |
| Load/expressions | 33 | -13.16% | 17 | -15.00% | 0 |  | 1,360 | +4.62% |
| Render/expressions | 2 | -50.00% | 1 | -75.00% | 0 |  | 286 | -6.84% |
| Load/filters | 157 | +13.77% | 82 | +5.13% | 0 |  | 2,048 | -0.39% |
| Render/filters | 26 | 0% | 5 | -37.50% | 0 |  | 773 | -0.26% |
| Load/for_filter_if | 15 | +25.00% | 6 | +20.00% | 0 |  | 1,076 | +4.36% |
| Render/for_filter_if | 1,653 | -0.06% | 59 | 0% | 0 |  | 125 | -9.42% |
| Load/for_loop_vars | 25 | -16.67% | 10 | -16.67% | 0 |  | 1,136 | +3.74% |
| Render/for_loop_vars | 4 | 0% | 1 | 0% | 0 |  | 108 | +4.85% |
| Load/for_range | 5 | 0% | 4 | 0% | 0 |  | 749 | +3.17% |
| Render/for_range | 1 | 0% | 1 | 0% | 0 |  | 117 | +28.57% |
| Load/html_autoescape | 484 | +0.41% | 501 | -0.60% | 0 |  | 4,048 | +5.77% |
| Render/html_autoescape | 4,926 | -12.05% | 1,558 | -1.14% | 0 |  | 31,409 | +6.51% |
| Load/inheritance | 6 | 0% | 5 | 0% | 0 |  | 679 | +9.34% |
| Render/inheritance | 580 | -1.53% | 121 | 0% | 0 |  | 813 | -2.98% |
| Load/int_arith | 86 | -4.44% | 60 | -4.76% | 0 |  | 2,003 | +2.09% |
| Render/int_arith | 12 | +33.33% | 4 | 0% | 0 |  | 698 | -4.25% |
| Load/large_static | 1,947 | +0.21% | 910 | +0.11% | 0 |  | 288 | +9.51% |
| Render/large_static | 708 | 0% | 568 | 0% | 0 |  | 14 | +250.00% |
| RenderSink/large_static | 1 | 0% | 0 |  | 0 |  | 3 | +50.00% |
| RenderTo/large_static | 686 | 0% | 568 | 0% | 0 |  | 4 | 0% |
| Load/macros | 71 | 0% | 57 | +7.55% | 0 |  | 1,643 | +0.67% |
| Render/macros | 721 | -0.55% | 211 | 0% | 0 |  | 1,238 | +118.34% |
| Load/many_tags | 28,081 | -0.11% | 22,213 | +0.46% | 0 |  | 253,089 | +4.58% |
| Render/many_tags | 6,132 | -0.05% | 856 | +0.94% | 0 |  | 154 | +24.19% |
| Load/mitsuhiko_table | 18 | +12.50% | 18 | +12.50% | 0 |  | 855 | +3.76% |
| Render/mitsuhiko_table | 9,027 | +0.02% | 6,601 | 0% | 0 |  | 315 | +3.62% |
| RenderSink/mitsuhiko_table | 9,006 | +0.08% | 1,498 | -0.07% | 0 |  | 239 | +4.37% |
| RenderTo/mitsuhiko_table | 9,034 | -0.02% | 6,608 | +0.02% | 0 |  | 272 | -5.56% |
| Load/mitsuhiko_table_wide | 88 | +2.33% | 78 | -9.30% | 0 |  | 960 | -6.25% |
| Render/mitsuhiko_table_wide | 9,151 | +0.14% | 23,036 | +0.03% | 0 |  | 308 | +7.32% |
| RenderSink/mitsuhiko_table_wide | 9,051 | +0.10% | 1,526 | -0.33% | 0 |  | 217 | -0.46% |
| RenderTo/mitsuhiko_table_wide | 9,143 | +0.09% | 23,000 | +0.02% | 0 |  | 260 | +7.44% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 30 | +100.00% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| RenderSink/plain_text | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| RenderTo/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 182 | -5.21% | 124 | -7.46% | 0 |  | 2,860 | +2.07% |
| Render/strings | 414 | -16.19% | 460 | -23.46% | 0 |  | 1,458 | -0.95% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 188 | +11.24% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| RenderSink/substitute | 0 |  | 0 |  | 0 |  | 3 | +200.00% |
| RenderTo/substitute | 0 |  | 0 |  | 0 |  | 5 | +66.67% |

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
