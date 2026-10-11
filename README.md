# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (71 so far, latest [6471f17](https://github.com/jinja2cpp/Jinja2Cpp/commit/6471f170a5c5d0e9ee549f70c6d6ddaabef4887f) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 372,168 | -0.49% |  | 59 | 0% |  | 19,920 | 0% | -49.03% |
| [Render/chat_llama](#renderchat_llama) | 407,277 | -0.19% |  | 355 | 0% |  | 15,312 | 0% | +6.63% |
| [Load/chat_mistral](#loadchat_mistral) | 451,269 | -0.41% |  | 57 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 639,488 | -0.56% |  | 421 | 0% |  | 12,184 | 0% | -7.75% |
| [Load/chat_qwen](#loadchat_qwen) | 292,499 | -0.42% |  | 45 | 0% |  | 15,872 | 0% | -41.92% |
| [Render/chat_qwen](#renderchat_qwen) | 389,581 | -0.06% |  | 320 | 0% |  | 9,400 | 0% | +10.54% |
| [Load/config_file](#loadconfig_file) | 102,743 | -0.26% |  | 27 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,649,806 | -0.45% |  | 1,124 | 0% |  | 25,376 | 0% | +6.59% |
| [Load/dict_ops](#loaddict_ops) | 34,140 | -0.14% | -76.90% | 9 | 0% | -93.13% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 346,354 | -1.12% | -49.87% | 114 | 0% | -74.67% | 36,168 | 0% | +4.24% |
| [Load/expressions](#loadexpressions) | 47,348 | +0.84% | -77.03% | 16 | 0% | -89.04% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 507,184 | -1.50% | -44.03% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 74,724 | -0.84% | -72.77% | 29 | 0% | -87.28% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 51,626 | -1.40% | -41.78% | 37 | 0% | -50.00% | 2,568 | 0% | -47.72% |
| [Load/for_filter_if](#loadfor_filter_if) | 30,457 | +0.04% | -77.50% | 7 | 0% | -94.31% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 333,480 | -2.06% | -61.47% | 6 | 0% | -97.35% | 13,904 | 0% | +281.98% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 41,884 | -0.15% | -76.82% | 10 | 0% | -93.63% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 264,715 | -2.90% | -75.32% | 4 | 0% | -99.23% | 2,128 | 0% | -23.12% |
| [Load/for_range](#loadfor_range) | 14,303 | +0.17% | -74.77% | 8 | 0% | -85.71% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,006 | +0.16% | -50.11% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 205,043 | -0.30% |  | 50 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,520,733 | -1.71% |  | 938 | 0% |  | 45,824 | 0% | +13.47% |
| [Load/inheritance](#loadinheritance) | 24,135 | -0.43% | -78.85% | 7 | 0% | -93.46% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 149,305 | -0.32% | -80.04% | 34 | 0% | -94.53% | 7,176 | 0% | +19.28% |
| [Load/large_static](#loadlarge_static) | 156,948 | 0% | -82.29% | 18 | 0% | -95.91% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,434 | -0.94% | -71.47% | 3 | 0% | -83.33% | 37,112 | 0% | -0.32% |
| [Load/macros](#loadmacros) | 58,084 | -0.29% | -74.52% | 28 | 0% | -84.95% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 862,029 | -1.94% | -66.37% | 312 | 0% | -88.57% | 20,304 | 0% | +37.93% |
| [Load/many_tags](#loadmany_tags) | 9,478,437 | -0.41% | -78.54% | 945 | 0% | -97.24% | 445,648 | 0% | -57.77% |
| [Render/many_tags](#rendermany_tags) | 1,101,654 | -3.50% | -47.11% | 15 | 0% | -28.57% | 82,520 | 0% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 49,647 | -0.63% | -79.10% | 20 | 0% | -89.13% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,453,211 | -0.01% | -47.77% | 35 | 0% | -99.14% | 373,872 | 0% | +7.74% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 68,567 | -0.30% |  | 45 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,803,901 | -0.01% |  | 53 | 0% |  | 1,407,264 | 0% | +1.90% |
| [Load/plain_text](#loadplain_text) | 3,655 | 0% | -62.23% | 4 | 0% | -77.78% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 987 | 0% | -85.48% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 75,655 | -0.63% | -74.08% | 30 | 0% | -86.61% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,030,168 | -2.34% | -50.89% | 979 | 0% | -53.69% | 18,384 | 0% | +2.54% |
| [Load/substitute](#loadsubstitute) | 7,780 | +0.03% | -73.94% | 6 | 0% | -83.78% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,706 | -0.26% | -69.30% | 3 | 0% | -75.00% | 600 | 0% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 884 | +2.67% | 859 | +0.47% | 0 |  | 8,327 | +12.39% |
| Render/chat_llama | 1,277 | +4.16% | 499 | +5.50% | 0 |  | 4,726 | -3.51% |
| Load/chat_mistral | 1,305 | -0.91% | 1,197 | -0.17% | 0 |  | 9,634 | +12.04% |
| Render/chat_mistral | 2,186 | +3.36% | 538 | +3.66% | 0 |  | 13,225 | -5.66% |
| Load/chat_qwen | 576 | -1.71% | 680 | +0.44% | 0 |  | 5,254 | +13.94% |
| Render/chat_qwen | 1,215 | +2.27% | 382 | +0.79% | 0 |  | 4,017 | -4.17% |
| Load/config_file | 173 | -0.57% | 155 | -4.32% | 0 |  | 3,039 | +7.12% |
| Render/config_file | 6,948 | +2.72% | 1,977 | -2.23% | 0 |  | 55,347 | -9.69% |
| Load/dict_ops | 22 | -4.35% | 16 | +60.00% | 0 |  | 1,337 | +4.95% |
| Render/dict_ops | 980 | +0.93% | 729 | +0.28% | 0 |  | 1,033 | -5.40% |
| Load/expressions | 37 | +27.59% | 29 | +26.09% | 0 |  | 1,252 | +0.48% |
| Render/expressions | 4 | 0% | 3 | 0% | 0 |  | 424 | -2.08% |
| Load/filters | 118 | -3.28% | 86 | -4.44% | 0 |  | 2,128 | -3.01% |
| Render/filters | 73 | +15.87% | 31 | +14.81% | 0 |  | 1,100 | -9.61% |
| Load/for_filter_if | 14 | -22.22% | 7 | -12.50% | 0 |  | 1,097 | +10.58% |
| Render/for_filter_if | 1,605 | 0% | 66 | 0% | 0 |  | 242 | -20.39% |
| Load/for_loop_vars | 20 | +33.33% | 13 | +116.67% | 0 |  | 1,131 | +7.10% |
| Render/for_loop_vars | 5 | 0% | 2 | 0% | 0 |  | 286 | -9.49% |
| Load/for_range | 6 | +200.00% | 5 | +25.00% | 0 |  | 750 | +5.34% |
| Render/for_range | 2 | 0% | 1 | 0% | 0 |  | 228 | +18.13% |
| Load/html_autoescape | 455 | +1.79% | 529 | +0.95% | 0 |  | 4,375 | +10.34% |
| Render/html_autoescape | 5,662 | +15.01% | 1,438 | -8.70% | 0 |  | 45,730 | -6.18% |
| Load/inheritance | 9 | +80.00% | 5 | +25.00% | 0 |  | 635 | +2.75% |
| Render/inheritance | 595 | +2.23% | 123 | -2.38% | 0 |  | 865 | -2.59% |
| Load/large_static | 1,960 | +0.05% | 887 | 0% | 0 |  | 250 | -9.75% |
| Render/large_static | 711 | 0% | 578 | 0% | 0 |  | 4 | -80.00% |
| Load/macros | 104 | +8.33% | 86 | 0% | 0 |  | 1,692 | +2.48% |
| Render/macros | 785 | +0.38% | 219 | -1.35% | 0 |  | 5,136 | +13.13% |
| Load/many_tags | 28,460 | +1.20% | 21,890 | +0.27% | 0 |  | 251,640 | +2.27% |
| Render/many_tags | 6,885 | +0.12% | 1,485 | +0.27% | 0 |  | 4,672 | -32.86% |
| Load/mitsuhiko_table | 40 | +11.11% | 39 | -7.14% | 0 |  | 890 | +3.97% |
| Render/mitsuhiko_table | 9,282 | +0.05% | 6,499 | -0.05% | 0 |  | 6,489 | +1038.42% |
| Load/mitsuhiko_table_wide | 90 | +2.27% | 115 | -3.36% | 0 |  | 1,396 | +12.67% |
| Render/mitsuhiko_table_wide | 9,401 | +0.16% | 23,006 | +0.03% | 0 |  | 6,508 | +997.47% |
| Load/plain_text | 0 |  | 2 | 0% | 0 |  | 16 | -46.67% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 5 | 0% |
| Load/strings | 155 | -3.73% | 122 | -4.69% | 0 |  | 2,912 | +6.98% |
| Render/strings | 927 | +0.11% | 1,356 | +0.44% | 0 |  | 1,860 | -6.91% |
| Load/substitute | 2 | 0% | 3 | 0% | 0 |  | 112 | -0.88% |
| Render/substitute | 0 |  | 0 |  | 0 |  | 4 | -20.00% |

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
