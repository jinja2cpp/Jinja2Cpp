# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (70 so far, latest [2fc6371](https://github.com/jinja2cpp/Jinja2Cpp/commit/2fc63711799b7d5630045c6a4e434876946f4c7c) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 373,991 | -27.62% |  | 59 | -35.87% |  | 19,920 | +0.32% | -49.03% |
| [Render/chat_llama](#renderchat_llama) | 408,036 | +0.20% |  | 355 | 0% |  | 15,312 | 0% | +6.63% |
| [Load/chat_mistral](#loadchat_mistral) | 453,132 | -25.84% |  | 57 | -24.00% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 643,119 | -0.01% |  | 421 | 0% |  | 12,184 | 0% | -7.75% |
| [Load/chat_qwen](#loadchat_qwen) | 293,725 | -25.69% |  | 45 | -33.82% |  | 15,872 | +0.71% | -41.92% |
| [Render/chat_qwen](#renderchat_qwen) | 389,797 | +0.02% |  | 320 | 0% |  | 9,400 | 0% | +10.54% |
| [Load/config_file](#loadconfig_file) | 103,007 | -24.63% |  | 27 | -20.59% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,657,262 | -0.17% |  | 1,124 | 0% |  | 25,376 | +0.13% | +6.59% |
| [Load/dict_ops](#loaddict_ops) | 34,189 | -23.19% | -76.86% | 9 | -25.00% | -93.13% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 350,269 | +0.13% | -49.30% | 114 | 0% | -74.67% | 36,168 | 0% | +4.24% |
| [Load/expressions](#loadexpressions) | 46,953 | -23.28% | -77.22% | 16 | -11.11% | -89.04% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 514,910 | 0% | -43.18% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 75,356 | -17.31% | -72.54% | 29 | -9.38% | -87.28% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,358 | +0.05% | -40.95% | 37 | 0% | -50.00% | 2,568 | 0% | -47.72% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,445 | -22.05% | -77.51% | 7 | -22.22% | -94.31% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 340,511 | -0.11% | -60.66% | 6 | 0% | -97.35% | 13,904 | 0% | +281.98% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,946 | -21.88% | -76.78% | 10 | -23.08% | -93.63% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,615 | +0.01% | -74.59% | 4 | 0% | -99.23% | 2,128 | 0% | -23.12% |
| [Load/for_range](#loadfor_range) | 14,279 | -22.35% | -74.81% | 8 | -20.00% | -85.71% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 45,932 | -0.06% | -50.19% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 205,662 | -23.71% |  | 50 | -18.03% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,547,147 | +0.03% |  | 938 | 0% |  | 45,824 | 0% | +13.47% |
| [Load/inheritance](#loadinheritance) | 24,239 | -26.31% | -78.76% | 7 | -30.00% | -93.46% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 149,789 | +0.03% | -79.97% | 34 | 0% | -94.53% | 7,176 | 0% | +19.28% |
| [Load/large_static](#loadlarge_static) | 156,946 | -28.94% | -82.29% | 18 | -35.71% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,637 | +0.07% | -71.20% | 3 | 0% | -83.33% | 37,112 | 0% | -0.32% |
| [Load/macros](#loadmacros) | 58,251 | -23.46% | -74.45% | 28 | -12.50% | -84.95% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 879,070 | +0.01% | -65.70% | 312 | 0% | -88.57% | 20,304 | 0% | +37.93% |
| [Load/many_tags](#loadmany_tags) | 9,517,734 | -23.03% | -78.45% | 945 | -1.77% | -97.24% | 445,648 | -0.01% | -57.77% |
| [Render/many_tags](#rendermany_tags) | 1,141,574 | 0% | -45.20% | 15 | 0% | -28.57% | 82,520 | 0% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 49,963 | -29.61% | -78.97% | 20 | -23.08% | -89.13% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,453,765 | 0% | -47.77% | 35 | 0% | -99.14% | 373,872 | 0% | +7.74% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 68,771 | -23.09% |  | 45 | -21.05% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,804,334 | 0% |  | 53 | 0% |  | 1,407,264 | 0% | +1.90% |
| [Load/plain_text](#loadplain_text) | 3,655 | -8.97% | -62.23% | 4 | -20.00% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 987 | 0% | -85.48% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 76,136 | -19.46% | -73.91% | 30 | -9.09% | -86.61% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,054,902 | 0% | -49.71% | 979 | 0% | -53.69% | 18,384 | 0% | +2.54% |
| [Load/substitute](#loadsubstitute) | 7,778 | -23.21% | -73.95% | 6 | -25.00% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,713 | +1.01% | -69.22% | 3 | 0% | -75.00% | 600 | 0% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 861 | -20.20% | 855 | -26.61% | 0 |  | 7,409 | -9.69% |
| Render/chat_llama | 1,226 | -5.47% | 473 | -6.15% | 0 |  | 4,898 | -12.69% |
| Load/chat_mistral | 1,317 | -21.23% | 1,199 | -28.72% | 0 |  | 8,599 | -8.91% |
| Render/chat_mistral | 2,115 | -1.44% | 519 | -5.29% | 0 |  | 14,019 | -5.94% |
| Load/chat_qwen | 586 | -26.38% | 677 | -38.12% | 0 |  | 4,611 | -13.78% |
| Render/chat_qwen | 1,188 | -1.98% | 379 | -5.01% | 0 |  | 4,192 | -4.12% |
| Load/config_file | 174 | -44.59% | 162 | -62.67% | 0 |  | 2,837 | -12.60% |
| Render/config_file | 6,764 | +3.50% | 2,022 | +0.55% | 0 |  | 61,283 | -0.20% |
| Load/dict_ops | 23 | -62.90% | 10 | -80.39% | 0 |  | 1,274 | -12.80% |
| Render/dict_ops | 971 | +1.89% | 727 | -1.22% | 0 |  | 1,092 | +1.02% |
| Load/expressions | 29 | -65.06% | 23 | -77.67% | 0 |  | 1,246 | -11.25% |
| Render/expressions | 4 | -42.86% | 3 | -25.00% | 0 |  | 433 | -4.63% |
| Load/filters | 122 | -32.97% | 90 | -25.00% | 0 |  | 2,194 | -10.96% |
| Render/filters | 63 | +43.18% | 27 | +8.00% | 0 |  | 1,217 | +5.83% |
| Load/for_filter_if | 18 | -37.93% | 8 | -61.90% | 0 |  | 992 | -13.13% |
| Render/for_filter_if | 1,605 | +0.94% | 66 | -5.71% | 0 |  | 304 | +1.33% |
| Load/for_loop_vars | 15 | -65.12% | 6 | -86.36% | 0 |  | 1,056 | -18.83% |
| Render/for_loop_vars | 5 | -37.50% | 2 | -60.00% | 0 |  | 316 | +1.61% |
| Load/for_range | 2 | -87.50% | 4 | -66.67% | 0 |  | 712 | -20.45% |
| Render/for_range | 2 | 0% | 1 | -50.00% | 0 |  | 193 | -9.81% |
| Load/html_autoescape | 447 | -27.67% | 524 | -33.08% | 0 |  | 3,965 | -15.49% |
| Render/html_autoescape | 4,923 | -2.65% | 1,575 | -3.61% | 0 |  | 48,742 | +0.34% |
| Load/inheritance | 5 | -50.00% | 4 | -66.67% | 0 |  | 618 | -28.14% |
| Render/inheritance | 582 | +0.17% | 126 | -0.79% | 0 |  | 888 | +1.60% |
| Load/large_static | 1,959 | -24.83% | 887 | -24.25% | 0 |  | 277 | -32.27% |
| Render/large_static | 711 | +0.14% | 578 | 0% | 0 |  | 20 | +400.00% |
| Load/macros | 96 | -45.76% | 86 | -61.95% | 0 |  | 1,651 | -11.33% |
| Render/macros | 782 | +0.51% | 222 | -3.90% | 0 |  | 4,540 | -26.70% |
| Load/many_tags | 28,122 | -22.76% | 21,830 | -23.92% | 0 |  | 246,055 | -14.94% |
| Render/many_tags | 6,877 | -0.16% | 1,481 | +1.65% | 0 |  | 6,959 | +177.47% |
| Load/mitsuhiko_table | 36 | -56.10% | 42 | -65.57% | 0 |  | 856 | -20.37% |
| Render/mitsuhiko_table | 9,277 | -0.24% | 6,502 | -0.05% | 0 |  | 570 | +11.11% |
| Load/mitsuhiko_table_wide | 88 | -46.99% | 119 | -52.96% | 0 |  | 1,239 | -24.54% |
| Render/mitsuhiko_table_wide | 9,386 | -0.14% | 23,000 | +0.02% | 0 |  | 593 | -87.79% |
| Load/plain_text | 0 |  | 2 | -33.33% | 0 |  | 30 | +130.77% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | +25.00% |
| Load/strings | 161 | -24.41% | 128 | -37.56% | 0 |  | 2,722 | -8.84% |
| Render/strings | 926 | +0.87% | 1,350 | +0.67% | 0 |  | 1,998 | -48.85% |
| Load/substitute | 2 | -71.43% | 3 | -70.00% | 0 |  | 113 | -64.24% |
| Render/substitute | 0 |  | 0 | -100.00% | 0 |  | 5 | +25.00% |

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

### Load/large_static

![Load/large_static](charts/Load_large_static.svg)

### Render/large_static

![Render/large_static](charts/Render_large_static.svg)

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

### Load/mitsuhiko_table_wide

![Load/mitsuhiko_table_wide](charts/Load_mitsuhiko_table_wide.svg)

### Render/mitsuhiko_table_wide

![Render/mitsuhiko_table_wide](charts/Render_mitsuhiko_table_wide.svg)

### Load/plain_text

![Load/plain_text](charts/Load_plain_text.svg)

### Render/plain_text

![Render/plain_text](charts/Render_plain_text.svg)

### Load/strings

![Load/strings](charts/Load_strings.svg)

### Render/strings

![Render/strings](charts/Render_strings.svg)

### Load/substitute

![Load/substitute](charts/Load_substitute.svg)

### Render/substitute

![Render/substitute](charts/Render_substitute.svg)
