# Benchmark trend

Instruction and allocation counts per iteration on master, one record per commit (68 so far, latest [523d59e](https://github.com/jinja2cpp/Jinja2Cpp/commit/523d59e0d5bf6abac0c714dcae161aee8595f2e5) on 2026-10-11). Written by the `trend` job of `.github/workflows/benchmark.yml` with `bench/trend.py`; the data is `history.jsonl`.

Memory is the bytes a loaded template keeps for `Load/*` and the peak heap use of one render for `Render/*`; vs first compares with the first record that has it.

| Benchmark | Instructions | vs previous | vs first | Allocations | vs previous | vs first | Memory | vs previous | vs first |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| [Load/chat_llama](#loadchat_llama) | 516,769 | 0% |  | 92 | 0% |  | 19,856 | 0% | -49.19% |
| [Render/chat_llama](#renderchat_llama) | 405,745 | -0.01% |  | 355 | 0% |  | 15,312 | 0% | +6.63% |
| [Load/chat_mistral](#loadchat_mistral) | 611,136 | 0% |  | 75 | 0% |  | 23,832 | 0% | -46.62% |
| [Render/chat_mistral](#renderchat_mistral) | 643,118 | +0.01% |  | 421 | 0% |  | 12,184 | 0% | -7.75% |
| [Load/chat_qwen](#loadchat_qwen) | 395,383 | 0% |  | 68 | 0% |  | 15,760 | 0% | -42.33% |
| [Render/chat_qwen](#renderchat_qwen) | 387,894 | -0.01% |  | 320 | 0% |  | 9,400 | 0% | +10.54% |
| [Load/config_file](#loadconfig_file) | 136,693 | 0% |  | 34 | 0% |  | 5,576 | 0% | -47.20% |
| [Render/config_file](#renderconfig_file) | 1,664,195 | -1.54% |  | 1,124 | 0% |  | 25,344 | +0.13% | +6.45% |
| [Load/dict_ops](#loaddict_ops) | 44,503 | 0% | -69.89% | 12 | 0% | -90.84% | 2,224 | 0% | -47.74% |
| [Render/dict_ops](#renderdict_ops) | 350,015 | -0.02% | -49.34% | 114 | 0% | -74.67% | 36,168 | 0% | +4.24% |
| [Load/expressions](#loadexpressions) | 61,193 | 0% | -70.32% | 18 | 0% | -87.67% | 3,560 | 0% | -41.98% |
| [Render/expressions](#renderexpressions) | 517,269 | -0.01% | -42.91% | 3 | 0% | -90.00% | 2,472 | 0% | -30.25% |
| [Load/filters](#loadfilters) | 91,015 | 0% | -66.84% | 32 | 0% | -85.96% | 3,896 | 0% | -65.87% |
| [Render/filters](#renderfilters) | 52,812 | 0% | -40.44% | 37 | 0% | -50.00% | 2,568 | 0% | -47.72% |
| [Load/for_filter_if](#loadfor_filter_if) | 39,072 | 0% | -71.14% | 9 | 0% | -92.68% | 1,936 | 0% | -43.85% |
| [Render/for_filter_if](#renderfor_filter_if) | 340,546 | -0.02% | -60.65% | 6 | 0% | -97.35% | 13,904 | 0% | +281.98% |
| [Load/for_loop_vars](#loadfor_loop_vars) | 53,739 | 0% | -70.25% | 13 | 0% | -91.72% | 2,416 | 0% | -46.55% |
| [Render/for_loop_vars](#renderfor_loop_vars) | 272,683 | -0.02% | -74.58% | 4 | 0% | -99.23% | 2,128 | 0% | -23.12% |
| [Load/for_range](#loadfor_range) | 18,390 | 0% | -67.56% | 10 | 0% | -82.14% | 1,136 | 0% | -43.20% |
| [Render/for_range](#renderfor_range) | 46,225 | -0.12% | -49.87% | 3 | 0% | -88.89% | 440 | 0% | -70.90% |
| [Load/html_autoescape](#loadhtml_autoescape) | 269,676 | 0% |  | 61 | 0% |  | 12,944 | 0% | -47.55% |
| [Render/html_autoescape](#renderhtml_autoescape) | 1,546,571 | -1.08% |  | 938 | 0% |  | 45,824 | +0.14% | +13.47% |
| [Load/inheritance](#loadinheritance) | 32,893 | 0% | -71.18% | 10 | 0% | -90.65% | 1,488 | 0% | -48.19% |
| [Render/inheritance](#renderinheritance) | 149,871 | 0% | -79.96% | 34 | 0% | -94.53% | 7,176 | 0% | +19.28% |
| [Load/large_static](#loadlarge_static) | 220,869 | 0% | -75.07% | 28 | 0% | -93.64% | 42,720 | 0% | -19.98% |
| [Render/large_static](#renderlarge_static) | 21,472 | 0% | -71.42% | 3 | 0% | -83.33% | 37,112 | 0% | -0.32% |
| [Load/macros](#loadmacros) | 76,096 | +0.05% | -66.62% | 32 | 0% | -82.80% | 5,128 | 0% | -17.50% |
| [Render/macros](#rendermacros) | 878,291 | -10.74% | -65.73% | 312 | 0% | -88.57% | 20,304 | +0.16% | +37.93% |
| [Load/many_tags](#loadmany_tags) | 12,357,046 | 0% | -72.02% | 962 | 0% | -97.19% | 445,680 | 0% | -57.76% |
| [Render/many_tags](#rendermany_tags) | 1,138,880 | 0% | -45.32% | 15 | 0% | -28.57% | 82,520 | 0% | +1207.35% |
| [Load/mitsuhiko_table](#loadmitsuhiko_table) | 70,988 | 0% | -70.12% | 26 | 0% | -85.87% | 3,552 | 0% | -41.42% |
| [Render/mitsuhiko_table](#rendermitsuhiko_table) | 6,557,161 | 0% | -46.93% | 35 | 0% | -99.14% | 373,872 | 0% | +7.74% |
| [Load/mitsuhiko_table_wide](#loadmitsuhiko_table_wide) | 89,416 | 0% |  | 57 | 0% |  | 5,736 | 0% | -30.59% |
| [Render/mitsuhiko_table_wide](#rendermitsuhiko_table_wide) | 6,927,626 | 0% |  | 53 | 0% |  | 1,407,264 | 0% | +1.90% |
| [Load/plain_text](#loadplain_text) | 4,015 | 0% | -58.51% | 5 | 0% | -72.22% | 688 | 0% | -38.57% |
| [Render/plain_text](#renderplain_text) | 987 | +0.10% | -85.48% | 1 | 0% | -91.67% | 40 | 0% | -94.44% |
| [Load/strings](#loadstrings) | 94,415 | 0% | -67.65% | 33 | 0% | -85.27% | 4,504 | 0% | -50.18% |
| [Render/strings](#renderstrings) | 1,055,680 | 0% | -49.67% | 979 | 0% | -53.69% | 18,384 | 0% | +2.54% |
| [Load/substitute](#loadsubstitute) | 10,129 | 0% | -66.07% | 8 | 0% | -78.38% | 832 | 0% | -45.26% |
| [Render/substitute](#rendersubstitute) | 2,685 | +0.04% | -69.54% | 3 | 0% | -75.00% | 600 | 0% | -16.67% |

## Cache misses

Misses per iteration in callgrind's cache model (`count.py --cache-sim`: 32 KB 8-way L1, 8 MB 16-way last level, no prefetcher); reported, not gated.

| Benchmark | D1 read misses | vs previous | D1 write misses | vs previous | LL data read misses | vs previous | I1 misses | vs previous |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Load/chat_llama | 1,075 | 0% | 1,157 | 0% | 0 |  | 7,945 | +1.11% |
| Render/chat_llama | 1,299 | +0.15% | 511 | 0% | 0 |  | 5,010 | +2.12% |
| Load/chat_mistral | 1,675 | 0% | 1,686 | 0% | 0 |  | 9,170 | +0.85% |
| Render/chat_mistral | 2,210 | +0.87% | 556 | +1.65% | 0 |  | 13,777 | +2.23% |
| Load/chat_qwen | 798 | 0% | 1,090 | 0% | 0 |  | 5,224 | +0.67% |
| Render/chat_qwen | 1,243 | -0.32% | 397 | +0.51% | 0 |  | 4,552 | +4.19% |
| Load/config_file | 316 | 0% | 432 | 0% | 0 |  | 3,255 | +0.99% |
| Render/config_file | 6,889 | -8.04% | 2,033 | -2.77% | 0 |  | 59,142 | -5.59% |
| Load/dict_ops | 64 | 0% | 47 | 0% | 0 |  | 1,465 | +1.17% |
| Render/dict_ops | 957 | -0.21% | 733 | +0.41% | 0 |  | 1,100 | -0.27% |
| Load/expressions | 82 | 0% | 96 | 0% | 0 |  | 1,442 | +2.49% |
| Render/expressions | 9 | 0% | 3 | +50.00% | 0 |  | 443 | -7.90% |
| Load/filters | 186 | 0% | 118 | 0% | 0 |  | 2,324 | -0.47% |
| Render/filters | 58 | -3.33% | 31 | -6.06% | 0 |  | 1,107 | -3.49% |
| Load/for_filter_if | 26 | 0% | 18 | 0% | 0 |  | 1,142 | -0.44% |
| Render/for_filter_if | 1,589 | -0.06% | 68 | -1.45% | 0 |  | 286 | +10.85% |
| Load/for_loop_vars | 40 | 0% | 38 | 0% | 0 |  | 1,273 | +1.11% |
| Render/for_loop_vars | 12 | 0% | 9 | 0% | 0 |  | 259 | -6.50% |
| Load/for_range | 10 | 0% | 10 | 0% | 0 |  | 896 | +1.24% |
| Render/for_range | 2 | 0% | 2 | 0% | 0 |  | 244 | +16.19% |
| Load/html_autoescape | 613 | 0% | 779 | 0% | 0 |  | 4,586 | -0.50% |
| Render/html_autoescape | 5,683 | +7.67% | 1,560 | +12.88% | 0 |  | 47,897 | -2.43% |
| Load/inheritance | 14 | 0% | 13 | 0% | 0 |  | 861 | +3.73% |
| Render/inheritance | 585 | +0.34% | 123 | -3.15% | 0 |  | 872 | -3.75% |
| Load/large_static | 2,602 | 0% | 1,171 | 0% | 0 |  | 391 | +8.61% |
| Render/large_static | 710 | -0.14% | 578 | -0.17% | 0 |  | 3 | -25.00% |
| Load/macros | 174 | 0% | 221 | 0% | 0 |  | 1,877 | +1.13% |
| Render/macros | 783 | -0.38% | 231 | +0.87% | 0 |  | 10,215 | +34.76% |
| Load/many_tags | 36,735 | 0% | 28,814 | 0% | 0 |  | 272,856 | +1.51% |
| Render/many_tags | 6,886 | +0.09% | 1,464 | +0.27% | 0 |  | 1,769 | -15.56% |
| Load/mitsuhiko_table | 76 | 0% | 121 | 0% | 0 |  | 1,084 | +2.17% |
| Render/mitsuhiko_table | 9,313 | 0% | 6,502 | -0.02% | 0 |  | 15,715 | +2647.38% |
| Load/mitsuhiko_table_wide | 167 | 0% | 253 | 0% | 0 |  | 1,607 | +11.52% |
| Render/mitsuhiko_table_wide | 9,414 | +0.02% | 22,996 | +0.01% | 0 |  | 15,072 | +2489.69% |
| Load/plain_text | 0 |  | 3 | 0% | 0 |  | 17 | +21.43% |
| Render/plain_text | 0 |  | 0 |  | 0 |  | 3 | -25.00% |
| Load/strings | 219 | 0% | 209 | 0% | 0 |  | 2,999 | +2.39% |
| Render/strings | 926 | -0.22% | 1,342 | -0.22% | 0 |  | 3,549 | +4.88% |
| Load/substitute | 5 | 0% | 10 | 0% | 0 |  | 323 | +9.49% |
| Render/substitute | 0 |  | 1 | 0% | 0 |  | 3 | -25.00% |

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
